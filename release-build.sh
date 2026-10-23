#!/usr/bin/env bash
# Portable Windows release build, run inside MSYS2 CLANG64 (called by release-build.ps1).
# Result: dist/BlockStart/ (run BlockStart.exe from there) and dist/BlockStart-<version>-windows-portable.zip
set -euo pipefail
cd "$(dirname "$0")"

JDK_DIR=$(ls -d /opt/jdk/*/ | head -n1)
export JAVA_HOME="${JDK_DIR%/}"
export PATH="$JAVA_HOME/bin:$PATH"

OUT=dist/BlockStart
VERSION=$(sed -n 's/^set(Launcher_VERSION_\(MAJOR\|MINOR\|PATCH\) \([0-9]*\))/\2/p' CMakeLists.txt | paste -sd.)
ZIP=BlockStart-$VERSION-windows-portable.zip

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

# the binary keeps its internal name (Microsoft login redirects to freesmlauncher://),
# only the file users click on is renamed; helper exes are found via the internal name
mv "$OUT/freesmlauncher.exe" "$OUT/BlockStart.exe"

rm -f "dist/$ZIP"
(cd dist && powershell -NoProfile -Command "Compress-Archive -Path BlockStart -DestinationPath $ZIP")
echo "Done: $OUT, dist/$ZIP"
