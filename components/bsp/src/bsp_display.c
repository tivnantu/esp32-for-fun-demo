#include "bsp_display.h"

#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_st77922.h"
#include "esp_log.h"

#include "st77922_init_cmds.h"

static const char *TAG = "bsp_display";

/* 显示接口定义，见规格书 03-display-st77922.md 第 2 节。 */
#define LCD_HOST              SPI2_HOST
#define LCD_H_RES             BSP_DISPLAY_WIDTH
#define LCD_V_RES             BSP_DISPLAY_HEIGHT
#define LCD_BITS_PER_PIXEL    16
#define LCD_MAX_TRANSFER_SZ   (LCD_H_RES * 80 * sizeof(uint16_t))

/* ST77922 为双栅极驱动，CASET 起止须 4 对齐。见规格书第 7 节。 */
_Static_assert(LCD_H_RES % 4 == 0, "面板宽度须为 4 的倍数");

static esp_lcd_panel_handle_t s_panel;
static bool s_initialized;

int bsp_display_width(void)
{
    return LCD_H_RES;
}

int bsp_display_height(void)
{
    return LCD_V_RES;
}

esp_err_t bsp_display_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "初始化 QSPI 总线");
    const spi_bus_config_t bus_config = ST77922_PANEL_BUS_QSPI_CONFIG(
        CONFIG_BSP_LCD_GPIO_PCLK, CONFIG_BSP_LCD_GPIO_D0, CONFIG_BSP_LCD_GPIO_D1, CONFIG_BSP_LCD_GPIO_D2,
        CONFIG_BSP_LCD_GPIO_D3, LCD_MAX_TRANSFER_SZ);
    ESP_RETURN_ON_ERROR(spi_bus_initialize(LCD_HOST, &bus_config, SPI_DMA_CH_AUTO), TAG, "QSPI 总线初始化失败");

    ESP_LOGI(TAG, "安装面板 IO");
    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_io_spi_config_t io_config = ST77922_PANEL_IO_QSPI_CONFIG(CONFIG_BSP_LCD_GPIO_CS, NULL, NULL);
    /* 宏默认值为 40 MHz，必须按板级配置覆盖为 80 MHz。见规格书第 5 节。 */
    io_config.pclk_hz = CONFIG_BSP_LCD_PIXEL_CLOCK_HZ;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io), TAG,
                        "面板 IO 创建失败");

    ESP_LOGI(TAG, "创建 ST77922 面板");
    const st77922_vendor_config_t vendor_config = {
        .init_cmds = st77922_init_cmds,
        .init_cmds_size = sizeof(st77922_init_cmds) / sizeof(st77922_init_cmds[0]),
        .flags = {.use_qspi_interface = 1},
    };
    const esp_lcd_panel_dev_config_t panel_config = {
        /* 面板复位与 ESP32-S3 EN 同源，无独立 GPIO，故为 -1。
         * 运行中无法单独复位面板。见规格书第 2 节。 */
        .reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = LCD_BITS_PER_PIXEL,
        .vendor_config = (void *)&vendor_config,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st77922(io, &panel_config, &s_panel), TAG, "面板创建失败");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel), TAG, "软复位失败");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel), TAG, "初始化序列下发失败");
    /* 初始化序列末尾已含 INVON(0x21)。此处重复下发使 IPS 反色要求显式可见，
     * 不依赖序列中的单条命令。该调用幂等；传 false 会下发 INVOFF(0x20) 撤销反色。 */
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(s_panel, true), TAG, "反色设置失败");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_panel, true), TAG, "开显示失败");

    s_initialized = true;
    ESP_LOGI(TAG, "就绪：%dx%d，QSPI %d Hz", LCD_H_RES, LCD_V_RES, CONFIG_BSP_LCD_PIXEL_CLOCK_HZ);
    return ESP_OK;
}

esp_err_t bsp_display_flush(int x, int y, int w, int h, const uint16_t *pixels)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (pixels == NULL || w <= 0 || h <= 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if ((x & 3) != 0 || ((x + w) & 3) != 0) {
        ESP_LOGE(TAG, "列范围未按 4 像素对齐：x %d..%d", x, x + w);
        return ESP_ERR_INVALID_ARG;
    }
    return esp_lcd_panel_draw_bitmap(s_panel, x, y, x + w, y + h, pixels);
}

esp_err_t bsp_display_flush_all(const uint16_t *pixels)
{
    return bsp_display_flush(0, 0, LCD_H_RES, LCD_V_RES, pixels);
}
