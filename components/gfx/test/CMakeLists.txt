cmake_minimum_required(VERSION 3.16)
project(gfx_host_test C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(test_gfx
    test_gfx.c
    ../src/gfx.c
)

target_include_directories(test_gfx PRIVATE ../include ../src)
target_compile_options(test_gfx PRIVATE -Wall -Wextra -Werror -O2)

enable_testing()
add_test(NAME gfx COMMAND test_gfx)
