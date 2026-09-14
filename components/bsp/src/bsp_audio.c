#include "bsp_audio.h"

#include <math.h>

#include "bsp_i2c.h"
#include "driver/i2s_std.h"
#include "esp_check.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "esp_log.h"

static const char *TAG = "bsp_audio";

#define AUDIO_I2S_PORT I2S_NUM_0

/* 单次写入的帧数。256 帧立体声 16 位 = 1024 字节。 */
#define AUDIO_CHUNK_FRAMES 256

/* 正弦表长度，取 2 的幂以便用移位取索引。 */
#define AUDIO_SINE_BITS 9
#define AUDIO_SINE_SIZE (1u << AUDIO_SINE_BITS)

/* 幅度取满量程的约 49%，留出余量，实际响度由编解码器增益控制。 */
#define AUDIO_AMPLITUDE 16000

static i2s_chan_handle_t s_tx;
static esp_codec_dev_handle_t s_codec;
/* 编解码是否处于打开状态。打开时功放使能，关闭时静音并禁用功放。 */
static bool s_opened;
static int16_t s_sine[AUDIO_SINE_SIZE];

static void audio_fill_sine_table(void)
{
    for (size_t i = 0; i < AUDIO_SINE_SIZE; i++) {
        const float radians = 2.0f * (float)M_PI * (float)i / (float)AUDIO_SINE_SIZE;
        s_sine[i] = (int16_t)(sinf(radians) * (float)AUDIO_AMPLITUDE);
    }
}

static esp_err_t audio_setup_i2s(void)
{
    const i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(AUDIO_I2S_PORT, I2S_ROLE_MASTER);
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &s_tx, NULL), TAG, "I2S 通道创建失败");

    const i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(CONFIG_BSP_AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg =
            {
                .mclk = CONFIG_BSP_AUDIO_I2S_MCLK,
                .bclk = CONFIG_BSP_AUDIO_I2S_BCLK,
                .ws = CONFIG_BSP_AUDIO_I2S_WS,
                .dout = CONFIG_BSP_AUDIO_I2S_DOUT,
                .din = CONFIG_BSP_AUDIO_I2S_DIN,
                .invert_flags =
                    {
                        .mclk_inv = false,
                        .bclk_inv = false,
                        .ws_inv = false,
                    },
            },
    };
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_tx, &std_cfg), TAG, "I2S 标准模式初始化失败");
    return i2s_channel_enable(s_tx);
}

static esp_err_t audio_setup_codec(void)
{
    i2c_master_bus_handle_t bus = bsp_i2c_bus();
    if (bus == NULL) {
        ESP_LOGE(TAG, "I2C 总线未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    const audio_codec_i2c_cfg_t i2c_cfg = {
        .port = 0,
        .addr = ES8311_CODEC_DEFAULT_ADDR,
        .bus_handle = bus,
        .clock_speed_hz = CONFIG_BSP_I2C_HZ,
    };
    const audio_codec_ctrl_if_t *ctrl_if = audio_codec_new_i2c_ctrl((audio_codec_i2c_cfg_t *)&i2c_cfg);
    if (ctrl_if == NULL) {
        ESP_LOGE(TAG, "I2C 控制接口创建失败");
        return ESP_FAIL;
    }

    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();
    if (gpio_if == NULL) {
        ESP_LOGE(TAG, "GPIO 接口创建失败");
        return ESP_FAIL;
    }

    const es8311_codec_cfg_t es_cfg = {
        .ctrl_if = ctrl_if,
        .gpio_if = gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .pa_pin = CONFIG_BSP_AUDIO_PA_GPIO,
        /* 功放使能为低电平有效。 */
        .pa_reverted = true,
        /* ESP32-S3 为 I2S 主设备，MCLK 由主控提供。 */
        .master_mode = false,
        .use_mclk = true,
        .digital_mic = false,
        .invert_mclk = false,
        .invert_sclk = false,
        .mclk_div = 0,
    };
    const audio_codec_if_t *codec_if = es8311_codec_new((es8311_codec_cfg_t *)&es_cfg);
    if (codec_if == NULL) {
        ESP_LOGE(TAG, "ES8311 编解码接口创建失败");
        return ESP_FAIL;
    }

    const audio_codec_i2s_cfg_t i2s_cfg = {
        .port = AUDIO_I2S_PORT,
        .rx_handle = NULL,
        .tx_handle = s_tx,
        .clk_src = 0,
    };
    const audio_codec_data_if_t *data_if = audio_codec_new_i2s_data((audio_codec_i2s_cfg_t *)&i2s_cfg);
    if (data_if == NULL) {
        ESP_LOGE(TAG, "I2S 数据接口创建失败");
        return ESP_FAIL;
    }

    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = codec_if,
        .data_if = data_if,
    };
    s_codec = esp_codec_dev_new(&dev_cfg);
    if (s_codec == NULL) {
        ESP_LOGE(TAG, "编解码设备创建失败");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "就绪：ES8311 地址 0x18，I2S%d %d Hz 立体声 16 位，功放 GPIO%d", (int)AUDIO_I2S_PORT,
             CONFIG_BSP_AUDIO_SAMPLE_RATE, CONFIG_BSP_AUDIO_PA_GPIO);
    return ESP_OK;
}

/* 打开编解码。已打开时无副作用。 */
static esp_err_t audio_open(void)
{
    if (s_opened) {
        return ESP_OK;
    }

    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = 16,
        .channel = 2,
        .channel_mask = 0,
        .sample_rate = CONFIG_BSP_AUDIO_SAMPLE_RATE,
        .mclk_multiple = 256,
    };
    const int ret = esp_codec_dev_open(s_codec, &fs);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "编解码设备打开失败：%d（编解码器可能无应答）", ret);
        return ESP_ERR_NOT_FOUND;
    }
    s_opened = true;
    return ESP_OK;
}

esp_err_t bsp_audio_init(void)
{
    /* 每一步各自判断状态，失败后重试不会重复创建已有资源。 */
    if (s_tx == NULL) {
        audio_fill_sine_table();
        ESP_RETURN_ON_ERROR(audio_setup_i2s(), TAG, "I2S 初始化失败");
    }
    if (s_codec == NULL) {
        ESP_RETURN_ON_ERROR(audio_setup_codec(), TAG, "编解码初始化失败");
    }
    if (!s_opened) {
        ESP_RETURN_ON_ERROR(audio_open(), TAG, "编解码打开失败");
        ESP_RETURN_ON_ERROR(bsp_audio_set_volume(70), TAG, "音量设置失败");
    }
    return ESP_OK;
}

bool bsp_audio_is_ready(void)
{
    return s_opened;
}

esp_err_t bsp_audio_set_volume(int percent)
{
    if (s_codec == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (percent < 0 || percent > 100) {
        return ESP_ERR_INVALID_ARG;
    }
    const int ret = esp_codec_dev_set_out_vol(s_codec, percent);
    return (ret == ESP_CODEC_DEV_OK) ? ESP_OK : ESP_FAIL;
}

esp_err_t bsp_audio_play_tone(uint32_t freq_hz, uint32_t duration_ms)
{
    if (s_codec == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (freq_hz < 20 || freq_hz > 8000 || duration_ms == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    const uint32_t rate = CONFIG_BSP_AUDIO_SAMPLE_RATE;
    const uint32_t total_frames = (uint32_t)(((uint64_t)rate * duration_ms) / 1000);
    /* 相位累加器为 32 位，取高 AUDIO_SINE_BITS 位查表。 */
    const uint32_t phase_step = (uint32_t)(((uint64_t)freq_hz << 32) / rate);
    const uint32_t sine_shift = 32 - AUDIO_SINE_BITS;

    int16_t buffer[AUDIO_CHUNK_FRAMES * 2];
    uint32_t phase = 0;
    uint32_t written_frames = 0;

    while (written_frames < total_frames) {
        uint32_t chunk = total_frames - written_frames;
        if (chunk > AUDIO_CHUNK_FRAMES) {
            chunk = AUDIO_CHUNK_FRAMES;
        }
        for (uint32_t i = 0; i < chunk; i++) {
            const int16_t sample = s_sine[phase >> sine_shift];
            buffer[i * 2] = sample;
            buffer[i * 2 + 1] = sample;
            phase += phase_step;
        }

        const size_t bytes = (size_t)chunk * 2 * sizeof(int16_t);
        if (esp_codec_dev_write(s_codec, buffer, (int)bytes) != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "写入失败");
            return ESP_FAIL;
        }
        written_frames += chunk;
    }
    return ESP_OK;
}

esp_err_t bsp_audio_set_mute(bool mute)
{
    if (s_codec == NULL || !s_opened) {
        return ESP_ERR_INVALID_STATE;
    }
    const int ret = esp_codec_dev_set_out_mute(s_codec, mute);
    return (ret == ESP_CODEC_DEV_OK) ? ESP_OK : ESP_FAIL;
}
