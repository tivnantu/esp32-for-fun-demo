/*
 * 触摸实现。
 *
 * 协议依据厂商 ST77922_TOUCH 库（资料包 1-示例程序_Demo/Arduino/
 * Install libraries/ST77922_TOUCH/ST77922_Touch.cpp）。本文件不臆造任何
 * 寄存器：地址、位域与初始化时序逐项对应厂商实现。
 *
 * 寄存器（16 位地址，大端）：
 *   0x0001 STATUS        低 4 位清零表示控制器就绪
 *   0x0009 MAX_TOUCHES   控制器支持的最大触点数
 *   0x0010 TOUCH_INFO    bit3 置位表示触点数据已更新
 *   0x0014 TOUCH_POINT0  触点数组，每点 7 字节，触点 N 在 0x0014 + N*7
 *
 * 触点帧（每点 7 字节，前 4 字节有用）：
 *   [0] bit7 存在标志，bit[5:0] X 高位
 *   [1] X 低 8 位
 *   [2] bit[5:0] Y 高位
 *   [3] Y 低 8 位
 *   [4..6] 未使用
 */

#include "bsp_touch.h"

#include "bsp_i2c.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "bsp_touch";

#define TOUCH_I2C_ADDR         0x55
#define TOUCH_REG_STATUS       0x0001
#define TOUCH_REG_MAX_TOUCHES  0x0009
#define TOUCH_REG_INFO         0x0010
#define TOUCH_REG_POINT0       0x0014
#define TOUCH_POINT_STRIDE     7
#define TOUCH_INFO_UPDATED     0x08
#define TOUCH_POINT_PRESENT    0x80
#define TOUCH_POINT_COORD_MASK 0x3F

#define TOUCH_RESET_DELAY_MS   100
#define TOUCH_READY_TIMEOUT_MS 1000
#define TOUCH_IO_TIMEOUT_MS    1000

static i2c_master_dev_handle_t s_dev;
static uint8_t s_max_points;
static bool s_initialized;

static esp_err_t touch_read(uint16_t reg, uint8_t *buf, size_t len)
{
    const uint8_t addr[2] = {(uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF)};
    return i2c_master_transmit_receive(s_dev, addr, sizeof(addr), buf, len, TOUCH_IO_TIMEOUT_MS);
}

esp_err_t bsp_touch_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    const gpio_config_t rst_cfg = {
        .pin_bit_mask = 1ULL << CONFIG_BSP_TOUCH_GPIO_RST,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&rst_cfg), TAG, "复位 GPIO 配置失败");

    const gpio_config_t int_cfg = {
        .pin_bit_mask = 1ULL << CONFIG_BSP_TOUCH_GPIO_INT,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&int_cfg), TAG, "中断 GPIO 配置失败");

    i2c_master_bus_handle_t bus = bsp_i2c_bus();
    if (bus == NULL) {
        ESP_LOGE(TAG, "I2C 总线未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    const i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TOUCH_I2C_ADDR,
        .scl_speed_hz = CONFIG_BSP_I2C_HZ,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &dev_cfg, &s_dev), TAG, "I2C 设备注册失败");

    /* 硬复位时序与厂商库一致：低 100 ms，高 100 ms。 */
    ESP_RETURN_ON_ERROR(gpio_set_level(CONFIG_BSP_TOUCH_GPIO_RST, 0), TAG, "复位拉低失败");
    vTaskDelay(pdMS_TO_TICKS(TOUCH_RESET_DELAY_MS));
    ESP_RETURN_ON_ERROR(gpio_set_level(CONFIG_BSP_TOUCH_GPIO_RST, 1), TAG, "复位拉高失败");
    vTaskDelay(pdMS_TO_TICKS(TOUCH_RESET_DELAY_MS));

    uint8_t status = 0xFF;
    int waited = 0;
    while (waited < TOUCH_READY_TIMEOUT_MS) {
        if (touch_read(TOUCH_REG_STATUS, &status, 1) == ESP_OK && (status & 0x0F) == 0) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
        waited += 10;
    }
    if ((status & 0x0F) != 0) {
        ESP_LOGE(TAG, "控制器未就绪，STATUS=0x%02X", status);
        return ESP_ERR_TIMEOUT;
    }

    uint8_t max_points = 0;
    ESP_RETURN_ON_ERROR(touch_read(TOUCH_REG_MAX_TOUCHES, &max_points, 1), TAG, "读取最大触点数失败");
    if (max_points == 0) {
        max_points = 1;
    } else if (max_points > BSP_TOUCH_MAX_POINTS) {
        max_points = BSP_TOUCH_MAX_POINTS;
    }
    s_max_points = max_points;

    s_initialized = true;
    ESP_LOGI(TAG, "就绪：I2C 地址 0x%02X，最大触点 %u", TOUCH_I2C_ADDR, s_max_points);
    return ESP_OK;
}

bool bsp_touch_is_ready(void)
{
    return s_initialized;
}

esp_err_t bsp_touch_poll(bsp_touch_state_t *state)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (state == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    state->updated = false;
    state->count = 0;

    uint8_t info = 0;
    ESP_RETURN_ON_ERROR(touch_read(TOUCH_REG_INFO, &info, 1), TAG, "读取更新标志失败");
    if ((info & TOUCH_INFO_UPDATED) == 0) {
        return ESP_OK;
    }
    state->updated = true;

    uint8_t raw[TOUCH_POINT_STRIDE * BSP_TOUCH_MAX_POINTS] = {0};
    ESP_RETURN_ON_ERROR(touch_read(TOUCH_REG_POINT0, raw, (size_t)TOUCH_POINT_STRIDE * s_max_points), TAG,
                       "读取触点失败");

    for (size_t i = 0; i < (size_t)s_max_points && state->count < BSP_TOUCH_MAX_POINTS; i++) {
        const uint8_t *p = &raw[i * TOUCH_POINT_STRIDE];
        if ((p[0] & TOUCH_POINT_PRESENT) == 0) {
            continue;
        }
        state->points[state->count].x = (uint16_t)(((p[0] & TOUCH_POINT_COORD_MASK) << 8) | p[1]);
        state->points[state->count].y = (uint16_t)(((p[2] & TOUCH_POINT_COORD_MASK) << 8) | p[3]);
        state->count++;
    }
    return ESP_OK;
}
