#include "bsp_i2c.h"

#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "bsp_i2c";

static i2c_master_bus_handle_t s_bus;

esp_err_t bsp_i2c_init(void)
{
    if (s_bus != NULL) {
        return ESP_OK;
    }

    const i2c_master_bus_config_t config = {
        .i2c_port = -1,
        .sda_io_num = CONFIG_BSP_I2C_GPIO_SDA,
        .scl_io_num = CONFIG_BSP_I2C_GPIO_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = CONFIG_BSP_I2C_GLITCH_IGNORE_CNT,
        .flags = {.enable_internal_pullup = true},
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&config, &s_bus), TAG, "I2C 总线创建失败");

    ESP_LOGI(TAG, "就绪：SDA IO%d，SCL IO%d，%d Hz", CONFIG_BSP_I2C_GPIO_SDA, CONFIG_BSP_I2C_GPIO_SCL,
             CONFIG_BSP_I2C_HZ);
    return ESP_OK;
}

i2c_master_bus_handle_t bsp_i2c_bus(void)
{
    return s_bus;
}
