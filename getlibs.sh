#!/usr/bin/env bash
set -eo pipefail

PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

if [ -z "$CMAKE_TOOLCHAIN_FILE" ]; then
    TARGET=$(uname --machine)-$(uname --kernel-name)
    TARGET="${TARGET,,}"
else
    TARGET=$(basename $CMAKE_TOOLCHAIN_FILE)
    TARGET=${TARGET%%.*}
fi

PREFIX="$PROJECT_DIR/local/$TARGET"
SOURCES="$PROJECT_DIR/local/sources"

[ -n "$CMAKE_BUILD_TYPE" ]     || export CMAKE_BUILD_TYPE=Release
[ -n "$CMAKE_PREFIX_PATH" ]    || export CMAKE_PREFIX_PATH="$PREFIX"
[ -n "$CMAKE_INSTALL_PREFIX" ] || export CMAKE_INSTALL_PREFIX="$PREFIX"

echo "  TARGET=$TARGET"
echo "  CMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE"
echo "  CMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH"
echo "  CMAKE_INSTALL_PREFIX=$CMAKE_INSTALL_PREFIX"

mkdir -p "$SOURCES"
cd "$SOURCES"

# doctest for unit testing
PACKAGE=doctest
VERSION=2.4.11
ARCHIVE="$PACKAGE-$VERSION.tar.gz"
SOURCE_DIR="$PACKAGE-$VERSION"
BUILD_DIR="build-${SOURCE_DIR}-$TARGET"
[ -r "$ARCHIVE" ] || curl -sL "https://github.com/doctest/doctest/archive/refs/tags/v${VERSION}.tar.gz" -o "$ARCHIVE"
[ -d "$SOURCE_DIR" ] || tar -xf "$ARCHIVE"
cmake  -S "$SOURCE_DIR" -B "$BUILD_DIR" -DDOCTEST_WITH_TESTS=OFF \
       -DDOCTEST_WITH_MAIN_IN_STATIC_LIB=ON -DDOCTEST_USE_STD_HEADERS=OFF
cmake --build "$BUILD_DIR" --config Release
cmake --install "$BUILD_DIR" --config Release --prefix "$CMAKE_INSTALL_PREFIX"
rm -Rf "$BUILD_DIR"
rm -Rf "$SOURCE_DIR"

# xxHash for fast high quality hashing
PACKAGE=xxHash
VERSION=0.8.2
ARCHIVE="$PACKAGE-$VERSION.tar.gz"
SOURCE_DIR="$PACKAGE-$VERSION"
BUILD_DIR="build-${SOURCE_DIR}-$TARGET"
[ -r "$ARCHIVE" ] || curl -sL "https://github.com/Cyan4973/xxHash/archive/refs/tags/v${VERSION}.tar.gz" -o "$ARCHIVE"
[ -d "$SOURCE_DIR" ] || tar -xf "$ARCHIVE"
echo "Building $SOURCE_DIR"
cmake -S "$SOURCE_DIR/cmake_unofficial" -B "$BUILD_DIR" -DBUILD_SHARED_LIBS=OFF
cmake --build "$BUILD_DIR" --config Release
cmake --install "$BUILD_DIR" --config Release --prefix="$CMAKE_INSTALL_PREFIX"
rm -Rf "$BUILD_DIR"
rm -Rf "$SOURCE_DIR"

# Boost
PACKAGE=boost
VERSION=1.83.0
ARCHIVE="$PACKAGE-$VERSION.tar.xz"
SOURCE_DIR="$PACKAGE-$VERSION"
BUILD_DIR="build-${SOURCE_DIR}-$TARGET"
[ -r "$ARCHIVE" ] || curl -sL "https://github.com/boostorg/boost/releases/download/${SOURCE_DIR}/${ARCHIVE}" -o "$ARCHIVE"
[ -d "$SOURCE_DIR" ] || tar -xf "$ARCHIVE"
echo "Building $SOURCE_DIR"
cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -DBOOST_ENABLE_CMAKE=ON -DBUILD_SHARED_LIBS=OFF \
      -DBOOST_INCLUDE_LIBRARIES="headers;math" -DBOOST_ENABLE_MPI=OFF -DBOOST_ENABLE_PYTHON=OFF \
      -DBOOST_RUNTIME_LINK=static -DBUILD_TESTING=OFF -DBOOST_USE_STATIC_LIBS=ON -DBOOST_USE_DEBUG_LIBS=ON \
      -DBOOST_USE_RELEASE_LIBS=ON -DBOOST_USE_STATIC_RUNTIME=ON -DBOOST_INSTALL_LAYOUT=system
cmake --build "$BUILD_DIR" --config Release
cmake --install "$BUILD_DIR" --config Release --prefix="$CMAKE_INSTALL_PREFIX"
rm -Rf "$BUILD_DIR"
rm -Rf "$SOURCE_DIR"

# UUtils various low level Uppaal utilities
#git clone https://github.com/UPPAALModelChecker/UUtils "$SOURCE_DIR/libs/sources/UUtils";
PACKAGE=UUtils
VERSION=2.0.0
ARCHIVE="$PACKAGE-$VERSION.tar.gz"
SOURCE_DIR="$PACKAGE-$VERSION"
BUILD_DIR="build-${SOURCE_DIR}-$TARGET"
[ -r "$ARCHIVE" ] || curl -sL "https://github.com/UPPAALModelChecker/UUtils/archive/refs/tags/v${VERSION}.tar.gz" -o "$ARCHIVE"
[ -d "$SOURCE_DIR" ] || tar -xf "$ARCHIVE"
echo "Building $SOURCE_DIR"
cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -DUUtils_WITH_TESTS=OFF -DUUtils_WITH_BENCHMARKS=OFF
cmake --build "$BUILD_DIR" --config Release
cmake --install "$BUILD_DIR" --config Release --prefix="$CMAKE_INSTALL_PREFIX"
rm -Rf "$BUILD_DIR"
rm -Rf "$SOURCE_DIR"
