#!/usr/bin/env bash
#
# 构建。
#
# 用法：
#   scripts/build.sh            构建出厂配置（全部可选子系统关闭）
#   scripts/build.sh --all      构建打开了全部可选子系统的配置
#
# 两套配置缺一不可。可选子系统在关闭时不参与编译，只有 --all 能保证
# 它们始终处于可编译状态，避免长期搁置后失效。

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if ! command -v idf.py >/dev/null 2>&1; then
    echo "错误：未找到 idf.py。" >&2
    echo "请先激活 ESP-IDF 环境，激活方式见 docs/04-build-and-flash.md。" >&2
    exit 1
fi

BUILD_DIR="build"
SDKCONFIG="sdkconfig"
DEFAULTS="sdkconfig.defaults"

if [[ "${1:-}" == "--all" ]]; then
    BUILD_DIR="build-all"
    SDKCONFIG="sdkconfig.all"
    DEFAULTS="sdkconfig.defaults;configs/all-subsystems.defaults"
    shift
fi

exec idf.py -B "$BUILD_DIR" -D SDKCONFIG="$SDKCONFIG" -D SDKCONFIG_DEFAULTS="$DEFAULTS" build "$@"
