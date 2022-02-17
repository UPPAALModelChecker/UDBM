#!/usr/bin/env bash
set -euxo pipefail

CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release"
if [ -z ${CMAKE_TOOLCHAIN_FILE+x} ]; then
	echo "Not using a custom toolchain";
else
	echo "Using toolchain $CMAKE_TOOLCHAIN_FILE";
	CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE";
fi

# Cursed line that should also work on macos
# https://stackoverflow.com/questions/59895/how-can-i-get-the-source-directory-of-a-bash-script-from-within-the-script-itsel
SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
mkdir -p "$SOURCE_DIR/libs/sources";

git clone https://github.com/UPPAALModelChecker/UUtils "$SOURCE_DIR/libs/sources/UUtils";
mkdir -p "$SOURCE_DIR/libs/sources/UUtils/build"
cd "$SOURCE_DIR/libs/sources/UUtils/build"
cmake $CMAKE_ARGS -DCMAKE_INSTALL_PREFIX="$SOURCE_DIR/libs/UUtils" ..
cmake --build . --config Release
cmake --install . --config Release
