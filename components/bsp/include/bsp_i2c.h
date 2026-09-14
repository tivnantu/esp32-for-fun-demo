/*
 * I2C 总线。
 *
 * 本模块是总线的唯一持有者，触摸控制器与音频编解码共用同一条总线
 * （硬件上 IO38/IO39 同时接到两者，见规格书 01-hardware.md 第 2.8 节）。
 *
 * 不要在其它模块中调用 i2c_new_master_bus()：同一组引脚重复创建总线
 * 会失败。
 *
 * esp_driver_i2c 的 master 驱动本身线程安全，多任务并发访问无需额外加锁。
 */

#pragma once

#include "esp_err.h"

#include "driver/i2c_master.h"

/* 创建总线。可重复调用，第二次起直接返回成功。 */
esp_err_t bsp_i2c_init(void);

/* 取得总线句柄，用于添加设备。未初始化时返回 NULL。 */
i2c_master_bus_handle_t bsp_i2c_bus(void);
