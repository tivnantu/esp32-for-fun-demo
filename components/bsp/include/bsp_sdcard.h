/*
 * MicroSD 卡（SDIO 4 位）。
 *
 * 引脚：CLK=IO5，CMD=IO4，D0=IO6，D1=IO7，D2=IO2，D3=IO3。
 *
 * 注意 DATA3 落在 GPIO3 上，该引脚同时是启动配置引脚（JTAG 源选择），
 * 见规格书 01-hardware.md 第 2.9 节。
 *
 * 挂载点固定为 /sdcard。卡不存在或识别失败时挂载失败，但不影响其余子系统。
 */

#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

/* 挂载点。 */
#define BSP_SDCARD_MOUNT_POINT "/sdcard"

/* 挂载文件系统。已挂载时直接返回成功。 */
esp_err_t bsp_sdcard_init(void);

bool bsp_sdcard_is_mounted(void);

/* 卸载文件系统。 */
esp_err_t bsp_sdcard_deinit(void);

/* 卡容量，单位 MiB。未挂载时返回 0。 */
uint32_t bsp_sdcard_capacity_mib(void);

/* 卡名称与容量描述，写入调用方缓冲区。未挂载时写入“未挂载”。 */
void bsp_sdcard_describe(char *buffer, size_t buffer_size);
