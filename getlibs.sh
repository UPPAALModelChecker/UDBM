#!/usr/bin/env bash
set -euxo pipefail

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

PREFIX="$SOURCE_DIR/local"
SOURCES="$SOURCE_DIR/local/sources"
mkdir -p "$SOURCES"

CMAKE_PREFIX_PATH="$PREFIX"

CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH -DCMAKE_INSTALL_PREFIX=$PREFIX"
if [ -z ${CMAKE_TOOLCHAIN_FILE+x} ]; then
	echo "Not using a custom toolchain";
else
	echo "Using toolchain $CMAKE_TOOLCHAIN_FILE";
	CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE";
fi

# doctest for unit testing
cd "$SOURCES"
curl -L https://github.com/doctest/doctest/archive/refs/tags/v2.4.9.tar.gz -o doctest-2.4.9.tar.gz
tar -xf doctest-2.4.9.tar.gz
mkdir -p "$SOURCES/doctest-2.4.9/build"
cd "$SOURCES/doctest-2.4.9/build"
cmake $CMAKE_ARGS ..
cmake --build . --config Release
cmake --install . --config Release

# xxHash for fast high quality hashing
cd "$SOURCES"
curl -L https://github.com/Cyan4973/xxHash/archive/refs/tags/v0.8.0.tar.gz -o xxHash-0.8.0.tar.gz
tar -xf xxHash-0.8.0.tar.gz
mkdir -p "$SOURCES/xxHash-0.8.0/build"
cd "$SOURCES/xxHash-0.8.0/build"
cmake $CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF ../cmake_unofficial
cmake --build . --config Release
cmake --install . --config Release

# UUtils various low level Uppaal utilities
#git clone https://github.com/UPPAALModelChecker/UUtils "$SOURCE_DIR/libs/sources/UUtils";
cd "$SOURCES"
curl -L https://github.com/UPPAALModelChecker/UUtils/archive/refs/tags/v1.1.1.tar.gz -o UUtils-1.1.1.tar.gz
tar -xf UUtils-1.1.1.tar.gz
mkdir -p "$SOURCES/UUtils-1.1.1/build"
cd "$SOURCES/UUtils-1.1.1/build"
cmake $CMAKE_ARGS ..
cmake --build . --config Release
cmake --install . --config Release
