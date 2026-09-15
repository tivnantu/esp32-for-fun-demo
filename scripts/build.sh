#!/usr/bin/env bash
#
# 构建。
#
# 用法：
#   scripts/build.sh            构建出厂配置（全部可选子系统关闭）
#   scripts/build.sh --all      构建打开了全部可选子系统的配置
#   scripts/build.sh --mixed    构建混合配置（touch 与 sdcard 单独开启）
#
# 三套配置的用途：
#   --all 与出厂配置覆盖门控的两个角，保证被关闭的代码始终可编译；
#   --mixed 让复合门控条件各由单侧开启满足，覆盖两个角之间的组合。

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# 解析 idf.py 的调用方式。
#
# Espressif 安装管理器（EIM）的激活脚本把 idf.py 定义成**含点号的 shell 函数**。
# bash 不把这类函数名传递给子进程，因此本脚本内 `idf.py` 不可见，而
# `IDF_PATH` / `IDF_PYTHON_ENV_PATH` 这两个环境变量可以正常继承。
# 这里依次尝试：PATH 中的 idf.py（官方 export.sh 的方式）、
# 环境变量拼出的解释器与脚本路径。
IDF_CMD=()
if command -v idf.py >/dev/null 2>&1; then
    IDF_CMD=(idf.py)
elif [[ -n "${IDF_PATH:-}" && -f "$IDF_PATH/tools/idf.py" ]]; then
    if [[ -n "${IDF_PYTHON_ENV_PATH:-}" && -x "$IDF_PYTHON_ENV_PATH/bin/python" ]]; then
        IDF_CMD=("$IDF_PYTHON_ENV_PATH/bin/python" "$IDF_PATH/tools/idf.py")
    else
        IDF_CMD=(python3 "$IDF_PATH/tools/idf.py")
    fi
else
    echo "错误：未找到 ESP-IDF。" >&2
    echo "请先激活 ESP-IDF 环境，例如：" >&2
    echo "  source <esp-idf 安装目录>/export.sh" >&2
    exit 1
fi

BUILD_DIR="build"
SDKCONFIG="sdkconfig"
DEFAULTS="sdkconfig.defaults"

case "${1:-}" in
--all)
    BUILD_DIR="build-all"
    SDKCONFIG="sdkconfig.all"
    DEFAULTS="sdkconfig.defaults;configs/all-subsystems.defaults"
    shift
    ;;
--mixed)
    BUILD_DIR="build-mixed"
    SDKCONFIG="sdkconfig.mixed"
    DEFAULTS="sdkconfig.defaults;configs/mixed.defaults"
    shift
    ;;
esac

exec "${IDF_CMD[@]}" -B "$BUILD_DIR" -D SDKCONFIG="$SDKCONFIG" -D SDKCONFIG_DEFAULTS="$DEFAULTS" build "$@"
