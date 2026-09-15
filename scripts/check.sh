#!/usr/bin/env bash
#
# 本地门禁。提交前运行。
#
# 检查项：
#   1. 宿主侧测试：gfx 单元测试 + 8 个硬件无关场景的渲染校验。
#   2. 逐套构建三套配置：
#        - 出厂配置：可选子系统全关
#        - 全子系统配置：可选子系统全开，保证被关闭的代码始终可编译
#        - 混合配置：复合门控条件各由单侧开启满足
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
echo "########## 混合配置构建 ##########"
"$ROOT/scripts/build.sh" --mixed

echo
echo "门禁全部通过。"
