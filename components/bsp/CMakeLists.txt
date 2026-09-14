set(bsp_srcs
    "src/bsp.c"
    "src/bsp_display.c"
    "src/bsp_backlight.c"
    "src/bsp_key.c"
)

# I2C 总线由触摸与音频共用，任一开启时都需要。
if(CONFIG_BSP_ENABLE_TOUCH OR CONFIG_BSP_ENABLE_AUDIO)
    list(APPEND bsp_srcs "src/bsp_i2c.c")
endif()

# 可选子系统按配置参与编译：关闭时不编译不链接，打开时同样接受编译检查。
if(CONFIG_BSP_ENABLE_TOUCH)
    list(APPEND bsp_srcs "src/bsp_touch.c")
endif()

if(CONFIG_BSP_ENABLE_RGB)
    list(APPEND bsp_srcs "src/bsp_rgb.c")
endif()

if(CONFIG_BSP_ENABLE_AUDIO)
    list(APPEND bsp_srcs "src/bsp_audio.c")
endif()

if(CONFIG_BSP_ENABLE_BATTERY)
    list(APPEND bsp_srcs "src/bsp_battery.c")
endif()

if(CONFIG_BSP_ENABLE_SDCARD)
    list(APPEND bsp_srcs "src/bsp_sdcard.c")
endif()

idf_component_register(
    SRCS ${bsp_srcs}
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES esp_lcd driver esp_driver_i2s esp_driver_sdmmc fatfs esp_adc esp_timer
)
