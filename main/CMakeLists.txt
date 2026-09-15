set(scene_srcs
    "app.c"
    "scenes/scene_orient.c"
    "scenes/scene_selftest.c"
    "scenes/scene_solid.c"
    "scenes/scene_pattern.c"
    "scenes/scene_edge.c"
    "scenes/scene_text.c"
    "scenes/scene_scroll.c"
    "scenes/scene_backlight.c"
)

# 可选子系统的演示场景随配置一同参与编译。
if(CONFIG_BSP_ENABLE_TOUCH)
    list(APPEND scene_srcs "scenes/scene_touch.c")
endif()

if(CONFIG_BSP_ENABLE_RGB)
    list(APPEND scene_srcs "scenes/scene_rgb.c")
endif()

if(CONFIG_BSP_ENABLE_AUDIO)
    list(APPEND scene_srcs "scenes/scene_audio.c")
endif()

if(CONFIG_BSP_ENABLE_STATUS_SCENE)
    list(APPEND scene_srcs "scenes/scene_status.c")
endif()

idf_component_register(
    SRCS ${scene_srcs}
    INCLUDE_DIRS "."
    REQUIRES bsp gfx esp_timer
)
