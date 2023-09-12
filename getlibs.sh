#!/usr/bin/env bash
set -eo pipefail

PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

if [ $# -eq 0 ]; then
    targets=$(uname --machine)-$(uname --kernel-name)
    targets="${targets,,}"
    echo "No targets specified on command line, assuming: $targets"
else
    targets="$@"
fi

SOURCES="$PROJECT_DIR/local/sources"
mkdir -p "$SOURCES"
cd "$SOURCES"

[ -n "$CMAKE_BUILD_TYPE" ]     || export CMAKE_BUILD_TYPE=Release

for target in $targets ; do
    PREFIX="$PROJECT_DIR/local/$target"

    export CMAKE_TOOLCHAIN_FILE="$PROJECT_DIR/cmake/toolchain/$target.cmake"
    export CMAKE_PREFIX_PATH="$PREFIX"
    export CMAKE_INSTALL_PREFIX="$PREFIX"

    # doctest for unit testing
    PACKAGE=doctest
    VERSION=2.4.11
    ARCHIVE="$PACKAGE-$VERSION.tar.gz"
    SOURCE_DIR="$PACKAGE-$VERSION"
    BUILD_DIR="build-${SOURCE_DIR}-$target"
    [ -r "$ARCHIVE" ] || curl -sL "https://github.com/doctest/doctest/archive/refs/tags/v${VERSION}.tar.gz" -o "$ARCHIVE"
    [ -d "$SOURCE_DIR" ] || tar -xf "$ARCHIVE"
    echo "Building $SOURCE_DIR"
    echo "  CMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE"
    echo "  CMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE"
    echo "  CMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH"
    echo "  CMAKE_INSTALL_PREFIX=$CMAKE_INSTALL_PREFIX"
    echo "  CMAKE_GENERATOR=$CMAKE_GENERATOR"
    cmake  -S "$SOURCE_DIR" -B "$BUILD_DIR" -DDOCTEST_WITH_TESTS=OFF \
           -DDOCTEST_WITH_MAIN_IN_STATIC_LIB=ON -DDOCTEST_USE_STD_HEADERS=OFF
    cmake --build "$BUILD_DIR" --config Release
    cmake --install "$BUILD_DIR" --config Release --prefix "$CMAKE_INSTALL_PREFIX"
    rm -Rf "$BUILD_DIR"
    rm -Rf "$SOURCE_DIR"

    # UUtils various low level Uppaal utilities
    #git clone https://github.com/UPPAALModelChecker/UUtils "$SOURCE_DIR/libs/sources/UUtils";
    PACKAGE=UUtils
    VERSION=2.0.3
    ARCHIVE="$PACKAGE-$VERSION.tar.gz"
    SOURCE_DIR="$PACKAGE-$VERSION"
    BUILD_DIR="build-${SOURCE_DIR}-$target"
    [ -r "$ARCHIVE" ] || curl -sL "https://github.com/UPPAALModelChecker/UUtils/archive/refs/tags/v${VERSION}.tar.gz" -o "$ARCHIVE"
    [ -d "$SOURCE_DIR" ] || tar -xf "$ARCHIVE"
#    git clone -b cmake-alias --single-branch --depth 1 https://github.com/mikucionisaau/UUtils.git "$SOURCE_DIR"
    echo "Building $SOURCE_DIR"
    echo "  CMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE"
    echo "  CMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE"
    echo "  CMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH"
    echo "  CMAKE_INSTALL_PREFIX=$CMAKE_INSTALL_PREFIX"
    echo "  CMAKE_GENERATOR=$CMAKE_GENERATOR"
    cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -DUUtils_WITH_TESTS=OFF -DUUtils_WITH_BENCHMARKS=OFF
    cmake --build "$BUILD_DIR" --config Release
    cmake --install "$BUILD_DIR" --config Release --prefix="$CMAKE_INSTALL_PREFIX"
    rm -Rf "$BUILD_DIR"
    rm -Rf "$SOURCE_DIR"
done
