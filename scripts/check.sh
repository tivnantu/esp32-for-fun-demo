#!/usr/bin/env bash
#
# 本地门禁。提交前运行。
#
# 检查项：
#   1. 宿主侧测试：gfx 单元测试 + 8 个硬件无关场景的渲染校验。
#   2. 出厂配置构建。
#   3. 全子系统配置构建：确保被 Kconfig 关闭的可选子系统代码也始终可编译。
#
# 任一项失败即以非零码退出。

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "########## 宿主侧测试 ##########"
"$ROOT/scripts/host-tests.sh"

echo
echo "########## 出厂配置构建 ##########"
"$ROOT/scripts/build.sh"

echo
echo "########## 全子系统配置构建 ##########"
"$ROOT/scripts/build.sh" --all

echo
echo "门禁全部通过。"
