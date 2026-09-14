#!/usr/bin/env bash
#
# 构建并烧录。
#
# 用法：
#   scripts/flash.sh [--all] [-p /dev/ttyACM0] [-b 460800]
#
# 默认端口 /dev/ttyACM0，对应 ESP32-S3 的原生 USB-Serial-JTAG。
# 该接口在本机上不会被 esptool 的自动探测列出，必须显式指定端口。
#
# 访问串口需要权限：把当前用户加入 uucp 组后重新登录即可免 sudo。
# 未生效时以 sudo 运行本脚本。

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

PORT="${ESPPORT:-/dev/ttyACM0}"
BAUD="${ESPBAUD:-460800}"
BUILD_ARGS=()

while [[ $# -gt 0 ]]; do
    case "$1" in
    --all)
        BUILD_ARGS+=("--all")
        shift
        ;;
    -p)
        PORT="$2"
        shift 2
        ;;
    -b)
        BAUD="$2"
        shift 2
        ;;
    *)
        echo "未知参数：$1" >&2
        exit 2
        ;;
    esac
done

"$ROOT/scripts/build.sh" "${BUILD_ARGS[@]}"

if [[ "${BUILD_ARGS[0]:-}" == "--all" ]]; then
    OUT="$ROOT/build-all"
else
    OUT="$ROOT/build"
fi

APP_BIN="$OUT/esp32_for_fun_demo.bin"
for artifact in "$OUT/bootloader/bootloader.bin" "$OUT/partition_table/partition-table.bin" "$APP_BIN"; do
    if [[ ! -f "$artifact" ]]; then
        echo "错误：缺少 $artifact" >&2
        exit 1
    fi
done

# 分区表约定的偏移：引导程序 0x0，分区表 0x8000，出厂应用 0x10000。
ESPTOOL_PY="$HOME/.espressif/tools/python/v5.5.4/venv/bin/python"
if command -v esptool.py >/dev/null 2>&1; then
    ESPTOOL=(esptool.py)
elif [[ -x "$ESPTOOL_PY" ]]; then
    ESPTOOL=("$ESPTOOL_PY" -m esptool)
else
    echo "错误：未找到 esptool。" >&2
    exit 1
fi

"${ESPTOOL[@]}" \
    --chip esp32s3 -b "$BAUD" --before default_reset --after hard_reset -p "$PORT" \
    write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m \
    0x0 "$OUT/bootloader/bootloader.bin" \
    0x8000 "$OUT/partition_table/partition-table.bin" \
    0x10000 "$APP_BIN"

echo "已烧录。串口日志："
echo "  python -m serial.tools.miniterm $PORT 115200"
