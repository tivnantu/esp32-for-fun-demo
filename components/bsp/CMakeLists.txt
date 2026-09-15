set(bsp_srcs
    "src/bsp.c"
    "src/bsp_display.c"
    "src/bsp_backlight.c"
    "src/bsp_key.c"
)

# I2C 总线由触摸与音频共用，任一开启时都需要。
if(CONFIG_BSP_ENABLE_I2C_BUS)
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
    # 按实际 include 的头文件所属组件精确声明依赖，不写 ESP-IDF 的遗留伞组件
    # driver。伞组件为向后兼容 REQUIRE 了全部 esp_driver_*（含 twai、mcpwm、
    # dac、parlio、sdspi 等本工程未用的驱动）；声明它是冗余的，且会让本组件
    # 的依赖面看起来比实际宽。
    #
    # 注意：去掉它并不会缩小构建图。driver 仍会进入构建，因为 esp_lcd 依赖它；
    # 本列表只影响本组件被记录与传递的依赖。
    #
    # 本列表不按 Kconfig 条件裁剪：IDF 对组件 CMakeLists 做两遍处理，第一遍
    # 在 Kconfig 变量可用之前就已注册组件，构建采纳的正是这一遍。条件化的
    # REQUIRES 会静默丢失依赖，表现为启用子系统后头文件找不到。
    REQUIRES
        esp_lcd
        esp_driver_gpio
        esp_driver_spi
        esp_driver_ledc
        esp_driver_i2c
        esp_driver_i2s
        esp_driver_sdmmc
        sdmmc
        fatfs
        esp_adc
)
