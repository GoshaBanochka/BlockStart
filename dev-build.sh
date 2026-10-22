#!/usr/bin/env bash
# Local Windows dev build, run inside MSYS2 CLANG64 (called by dev-build.ps1).
# Usage: dev-build.sh [--clean]
set -euo pipefail
cd "$(dirname "$0")"

JDK_DIR=$(ls -d /opt/jdk/*/ | head -n1)
export JAVA_HOME="${JDK_DIR%/}"
export PATH="$JAVA_HOME/bin:$PATH"

if [ "${1:-}" = "--clean" ]; then
    rm -rf build-dev install-dev
fi

if [ ! -f build-dev/build.ninja ]; then
    cmake -S . -B build-dev -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_LTO=OFF \
        -DLauncher_ENABLE_JAVA_DOWNLOADER=ON -DCMAKE_INSTALL_PREFIX=install-dev \
        -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
fi

cmake --build build-dev
cmake --install build-dev > /dev/null
