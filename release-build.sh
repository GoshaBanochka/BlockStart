#!/usr/bin/env bash
# Portable Windows release build, run inside MSYS2 CLANG64 (called by release-build.ps1).
# Result: dist/FreesmLauncher-portable/ (run freesmlauncher.exe from there) and a zip of it.
set -euo pipefail
cd "$(dirname "$0")"

JDK_DIR=$(ls -d /opt/jdk/*/ | head -n1)
export JAVA_HOME="${JDK_DIR%/}"
export PATH="$JAVA_HOME/bin:$PATH"

OUT=dist/FreesmLauncher-portable

# configure every time: the install script bakes in CMAKE_INSTALL_PREFIX, so it must point at $OUT
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_LTO=OFF \
        -DCMAKE_INSTALL_PREFIX="$(pwd)/$OUT" \
        -DLauncher_ENABLE_JAVA_DOWNLOADER=ON -DLauncher_BUILD_PLATFORM=win-mingw-portable \
        -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

cmake --build build-release

# the install step copies the exe together with Qt, its plugins and all runtime DLLs
rm -rf "$OUT"
cmake --install build-release
cmake --install build-release --component portable

rm -f dist/FreesmLauncher-portable.zip
(cd dist && powershell -NoProfile -Command "Compress-Archive -Path FreesmLauncher-portable -DestinationPath FreesmLauncher-portable.zip")
echo "Done: $OUT"
