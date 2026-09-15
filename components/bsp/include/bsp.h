/*
 * 板级支持包统一入口。
 */

#pragma once

#include "esp_err.h"

#include "bsp_backlight.h"
#include "bsp_display.h"
#include "bsp_key.h"

#if CONFIG_BSP_ENABLE_I2C_BUS
#include "bsp_i2c.h"
#endif

#if CONFIG_BSP_ENABLE_TOUCH
#include "bsp_touch.h"
#endif

#if CONFIG_BSP_ENABLE_RGB
#include "bsp_rgb.h"
#endif

#if CONFIG_BSP_ENABLE_AUDIO
#include "bsp_audio.h"
#endif

#if CONFIG_BSP_ENABLE_BATTERY
#include "bsp_battery.h"
#endif

#if CONFIG_BSP_ENABLE_SDCARD
#include "bsp_sdcard.h"
#endif

/*
 * 初始化已启用的子系统。背光保持熄灭。
 * 可选子系统在 menuconfig 中关闭时不参与编译，也不占用引脚。
 */
esp_err_t bsp_init(void);
