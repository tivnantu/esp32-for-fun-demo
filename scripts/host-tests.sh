#!/usr/bin/env bash
#
# 宿主侧测试。不需要硬件，也不需要 ESP-IDF 环境。
#
# 两部分：
#   1. gfx 组件的单元测试：字形渲染、画布裁剪、跨越式画布写入等。
#   2. 场景渲染校验：与设备上共用同一份场景源码，在宿主机上渲染整屏，
#      逐像素断言布局不变量，并输出字符图便于人工查看。

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${HOST_BUILD_DIR:-${TMPDIR:-/tmp}/esp32-for-fun-demo-host}"

# 宿主侧测试使用宿主编译器，因此必须把 ESP-IDF 的交叉工具链从 PATH 中剥离。
# 激活 ESP-IDF 后 PATH 里存在交叉汇编器（as），宿主的 cc 会通过 PATH 找到它
# 并以 --64 参数调用，于是编译简单测试程序都会失败：
#   as: unrecognized option '--64'
# 本脚本可能在已激活 ESP-IDF 的 shell 中运行（scripts/check.sh 就是如此），
# 因此这里显式只保留宿主侧工具目录。
_old_ifs="$IFS"
IFS=':'
HOST_PATH=""
for _entry in $PATH; do
    case "$_entry" in
    # 保留 ESP-IDF 提供的宿主侧工具。
    */.espressif/tools/cmake/* | */.espressif/tools/ninja/* | */.espressif/tools/python/*) ;;
    # 丢弃其余 ESP-IDF 工具目录，它们都是交叉工具链。
    */.espressif/tools/*) continue ;;
    esac
    HOST_PATH="${HOST_PATH:+$HOST_PATH:}$_entry"
done
IFS="$_old_ifs"
export PATH="$HOST_PATH"

command -v cmake >/dev/null 2>&1 || {
    echo "错误：未找到 cmake。" >&2
    exit 1
}

echo "== gfx 单元测试 =="
cmake -S "$ROOT/components/gfx/test" -B "$BUILD/gfx" >/dev/null
cmake --build "$BUILD/gfx" >/dev/null
"$BUILD/gfx/test_gfx"

echo
echo "== 场景渲染校验 =="
cmake -S "$ROOT/tools/preview" -B "$BUILD/preview" >/dev/null
cmake --build "$BUILD/preview" >/dev/null

# 只有不依赖硬件的场景参与宿主侧校验：其余场景需要真实外设。
SCENES=(orient selftest solid pattern edge text scroll backlight)
FAILED=0
for scene in "${SCENES[@]}"; do
    line="$("$BUILD/preview/preview" "$scene" | tail -1)"
    echo "  $line"
    if [[ "$line" != *"：0 项失败" ]]; then
        FAILED=1
    fi
done

if [[ $FAILED -ne 0 ]]; then
    echo "宿主侧校验失败。" >&2
    exit 1
fi

echo
echo "宿主侧测试全部通过。"
