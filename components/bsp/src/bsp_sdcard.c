#include "bsp_sdcard.h"

#include <stdio.h>

#include "driver/sdmmc_host.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

static const char *TAG = "bsp_sdcard";

#define SDCARD_MAX_FILES 5
#define SDCARD_ALLOC_UNIT_SIZE (16 * 1024)

/* SDIO 总线位宽。原理图为 4 位接线。 */
#define SDCARD_BUS_WIDTH 4

static sdmmc_card_t *s_card;
static bool s_mounted;

esp_err_t bsp_sdcard_init(void)
{
    if (s_mounted) {
        return ESP_OK;
    }

    const esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        /* 不自动格式化：格式化会静默破坏卡上既有数据。 */
        .format_if_mount_failed = false,
        .max_files = SDCARD_MAX_FILES,
        .allocation_unit_size = SDCARD_ALLOC_UNIT_SIZE,
    };

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.clk = (gpio_num_t)CONFIG_BSP_SD_GPIO_CLK;
    slot.cmd = (gpio_num_t)CONFIG_BSP_SD_GPIO_CMD;
    slot.d0 = (gpio_num_t)CONFIG_BSP_SD_GPIO_D0;
    slot.d1 = (gpio_num_t)CONFIG_BSP_SD_GPIO_D1;
    slot.d2 = (gpio_num_t)CONFIG_BSP_SD_GPIO_D2;
    slot.d3 = (gpio_num_t)CONFIG_BSP_SD_GPIO_D3;
    slot.width = SDCARD_BUS_WIDTH;
    /* 仅供调试：内部上拉不足以保证信号完整性，正式硬件应有外部上拉。 */
    slot.flags = SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    ESP_RETURN_ON_ERROR(esp_vfs_fat_sdmmc_mount(BSP_SDCARD_MOUNT_POINT, &host, &slot, &mount_cfg, &s_card), TAG,
                        "挂载失败");
    s_mounted = true;

    ESP_LOGI(TAG, "就绪：%s，%u MiB，%d 位总线，CLK IO%d CMD IO%d", BSP_SDCARD_MOUNT_POINT,
             (unsigned)bsp_sdcard_capacity_mib(), SDCARD_BUS_WIDTH, CONFIG_BSP_SD_GPIO_CLK, CONFIG_BSP_SD_GPIO_CMD);
    return ESP_OK;
}

bool bsp_sdcard_is_mounted(void)
{
    return s_mounted;
}

esp_err_t bsp_sdcard_deinit(void)
{
    if (!s_mounted) {
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(esp_vfs_fat_sdcard_unmount(BSP_SDCARD_MOUNT_POINT, s_card), TAG, "卸载失败");
    s_card = NULL;
    s_mounted = false;
    return ESP_OK;
}

uint32_t bsp_sdcard_capacity_mib(void)
{
    if (!s_mounted || s_card == NULL) {
        return 0;
    }
    /* cid/sdmmc_csd 中的容量单位为字节。 */
    const uint64_t bytes = (uint64_t)s_card->csd.capacity * s_card->csd.sector_size;
    return (uint32_t)(bytes / (1024u * 1024u));
}

void bsp_sdcard_describe(char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        return;
    }
    if (!s_mounted || s_card == NULL) {
        snprintf(buffer, buffer_size, "未挂载");
        return;
    }
    snprintf(buffer, buffer_size, "%s %u MiB", s_card->cid.name, (unsigned)bsp_sdcard_capacity_mib());
}
