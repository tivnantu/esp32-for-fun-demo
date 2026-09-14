cmake_minimum_required(VERSION 3.16)
project(scene_preview C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 场景源文件直接参与宿主侧编译，与设备上使用的是同一份代码。
add_executable(preview
    preview.c
    stubs/bsp_stub.c
    ../../main/scenes/scene_orient.c
    ../../main/scenes/scene_selftest.c
    ../../main/scenes/scene_solid.c
    ../../main/scenes/scene_pattern.c
    ../../main/scenes/scene_edge.c
    ../../main/scenes/scene_text.c
    ../../main/scenes/scene_scroll.c
    ../../main/scenes/scene_backlight.c
    ../../components/gfx/src/gfx.c
)

target_include_directories(preview PRIVATE
    stubs
    ../../main
    ../../components/bsp/include
    ../../components/gfx/include
    ../../components/gfx/src
)

target_compile_options(preview PRIVATE -Wall -Wextra -Werror -O2)
