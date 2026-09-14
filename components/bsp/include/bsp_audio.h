/*
 * 音频输出。
 *
 * 链路：ESP32-S3 I2S ──► ES8311（编解码，I2C 控制）──► FM8002E（功放）──► 喇叭。
 *
 * 编解码寄存器序列由 Espressif esp_codec_dev 组件提供，本模块不手写寄存器。
 * 功放使能引脚交给 ES8311 驱动管理（pa_pin / pa_reverted），不在本模块重复控制。
 *
 * 编解码 I2C 从地址 0x18（esp_codec_dev 中的常量 ES8311_CODEC_DEFAULT_ADDR 为
 * 0x30，即含读写位的 8 位形式），与触摸控制器共用 I2C 总线。
 */

#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * 初始化 I2S 与 ES8311 并打开输出。可重复调用，已打开时无副作用。
 * 返回 ESP_ERR_NOT_FOUND 表示编解码器无应答（I2C 地址错误或器件未焊接）。
 */
esp_err_t bsp_audio_init(void);

/* 编解码是否已成功打开。 */
bool bsp_audio_is_ready(void);

/* 设置输出音量，百分比 0..100。 */
esp_err_t bsp_audio_set_volume(int percent);

/*
 * 静音或取消静音输出。
 *
 * 停止发声用静音而不是关闭编解码。esp_codec_dev_close() 会禁用 I2S 通道，
 * 而下一次 esp_codec_dev_open() 内部的 set_fmt 会对已禁用的通道再次调用
 * i2s_channel_disable；IDF 的 i2s_channel_disable 对未启用的通道打错误日志，
 * 于是每次“关闭后再打开”都会产生一条无害但干扰判读的 E 行。
 * 静音直接写 ES8311 的 DAC 静音位，输出置零，不涉及通道状态。
 */
esp_err_t bsp_audio_set_mute(bool mute);

/*
 * 播放单频正弦波。
 * freq_hz 有效范围 20..8000（采样率 16 kHz 下的可听范围）。
 */
esp_err_t bsp_audio_play_tone(uint32_t freq_hz, uint32_t duration_ms);
