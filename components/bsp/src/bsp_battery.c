#include "bsp_battery.h"

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "bsp_battery";

/* 硬件分压比为 2:1。 */
#define BATTERY_DIVIDER_NUMERATOR 2

/* 多次采样取平均，抑制 ADC 噪声。 */
#define BATTERY_SAMPLE_COUNT 16

static adc_oneshot_unit_handle_t s_adc;
static adc_cali_handle_t s_cali;
static bool s_cali_valid;

static adc_unit_t battery_adc_unit(void)
{
    return (CONFIG_BSP_BATTERY_ADC_UNIT == 2) ? ADC_UNIT_2 : ADC_UNIT_1;
}

esp_err_t bsp_battery_init(void)
{
    if (s_adc != NULL) {
        return ESP_OK;
    }

    const adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = battery_adc_unit(),
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&unit_cfg, &s_adc), TAG, "ADC 单元创建失败");

    const adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_config_channel(s_adc, CONFIG_BSP_BATTERY_ADC_CHANNEL, &chan_cfg), TAG,
                        "ADC 通道配置失败");

    const adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = battery_adc_unit(),
        .chan = CONFIG_BSP_BATTERY_ADC_CHANNEL,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    s_cali_valid = (adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_cali) == ESP_OK);
    if (!s_cali_valid) {
        ESP_LOGW(TAG, "校准方案不可用，将按满量程比例换算");
    }

    ESP_LOGI(TAG, "就绪：ADC%d 通道 %d，分压比 %d:1，校准 %s", CONFIG_BSP_BATTERY_ADC_UNIT,
             CONFIG_BSP_BATTERY_ADC_CHANNEL, BATTERY_DIVIDER_NUMERATOR, s_cali_valid ? "可用" : "不可用");
    return ESP_OK;
}

esp_err_t bsp_battery_read_mv(int *out_mv)
{
    if (s_adc == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (out_mv == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int32_t sum = 0;
    for (int i = 0; i < BATTERY_SAMPLE_COUNT; i++) {
        int raw = 0;
        ESP_RETURN_ON_ERROR(adc_oneshot_read(s_adc, CONFIG_BSP_BATTERY_ADC_CHANNEL, &raw), TAG, "ADC 读取失败");
        sum += raw;
    }
    const int raw_avg = (int)(sum / BATTERY_SAMPLE_COUNT);

    int pin_mv;
    if (s_cali_valid) {
        ESP_RETURN_ON_ERROR(adc_cali_raw_to_voltage(s_cali, raw_avg, &pin_mv), TAG, "ADC 换算失败");
    } else {
        /* 无校准方案时按 12 位满量程与 12 dB 衰减的标称上限换算。 */
        pin_mv = (int)(((int64_t)raw_avg * 3100 * BATTERY_DIVIDER_NUMERATOR) / 4095);
    }

    *out_mv = pin_mv * BATTERY_DIVIDER_NUMERATOR;
    return ESP_OK;
}

int bsp_battery_read_mv_or_zero(void)
{
    int mv = 0;
    if (bsp_battery_read_mv(&mv) != ESP_OK) {
        return 0;
    }
    return mv;
}
