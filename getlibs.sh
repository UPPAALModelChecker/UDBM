#!/usr/bin/env bash
set -eo pipefail

PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

if [ $# -eq 0 ]; then
  echo "Script $0 fetches, compiles and installs dependent libraries."
  machine=$(uname --machine)
  kernel=$(uname --kernel-name)
  targets=${machine,,}-${kernel,,}
  echo "Expects targets as arguments, for example: $targets"
  exit 1
else
  targets="$@"
fi

SOURCES="$PROJECT_DIR/local/sources"
mkdir -p "$SOURCES"

if [ -z "${CMAKE_BUILD_TYPE+x}" ]; then
    export CMAKE_BUILD_TYPE=Release
elif [ "$CMAKE_BUILD_TYPE" != Release ]; then
    echo "WARNING: building libs with CMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE"
fi

for target in $targets ; do
    PREFIX="$PROJECT_DIR/local/$target"
    export CMAKE_TOOLCHAIN_FILE="$PROJECT_DIR/cmake/toolchain/$target.cmake"
    export CMAKE_PREFIX_PATH="$PREFIX"
    export CMAKE_INSTALL_PREFIX="$PREFIX"

    # doctest for unit testing
    NAME=doctest
    VERSION=2.4.11
    LIBRARY="${NAME}-${VERSION}"
    if [ -r "$PREFIX/include/doctest/doctest.h" ] ; then
      echo "$LIBRARY is already installed in $PREFIX"
    else
      ARCHIVE="${LIBRARY}.tar.gz"
      SHA256=632ed2c05a7f53fa961381497bf8069093f0d6628c5f26286161fbd32a560186
      SOURCE="${SOURCES}/${LIBRARY}"
      BUILD="${PREFIX}/build-${LIBRARY}"
      pushd "$SOURCES"
      [ -r "$ARCHIVE" ] || curl -sL "https://github.com/doctest/doctest/archive/refs/tags/v${VERSION}.tar.gz" -o "$ARCHIVE"
      if [ -n "$(command -v sha256sum)" ]; then echo "$SHA256 $ARCHIVE" | sha256sum --check ; fi
      [ -d "$SOURCE" ] || tar -xf "$ARCHIVE"
      popd
      echo "Building $LIBRARY in $BUILD"
      echo "  CMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE"
      echo "  CMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE"
      echo "  CMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH"
      echo "  CMAKE_INSTALL_PREFIX=$CMAKE_INSTALL_PREFIX"
      echo "  CMAKE_GENERATOR=$CMAKE_GENERATOR"
      cmake  -S "$SOURCE" -B "$BUILD" -DDOCTEST_WITH_TESTS=OFF \
           -DDOCTEST_WITH_MAIN_IN_STATIC_LIB=ON -DDOCTEST_USE_STD_HEADERS=OFF
      cmake --build "$BUILD" --config $CMAKE_BUILD_TYPE
      cmake --install "$BUILD" --config $CMAKE_BUILD_TYPE --prefix "$CMAKE_INSTALL_PREFIX"
      rm -Rf "$BUILD"
      rm -Rf "$SOURCE"
    fi

    # UUtils various low level Uppaal utilities
    NAME=UUtils
    VERSION=2.0.5
    LIBRARY="${NAME}-${VERSION}"
    if [ -r "$PREFIX/include/base/Enumerator.h" ]; then
      echo "$LIBRARY is already installed in $PREFIX"
    else
      ARCHIVE="${LIBRARY}.tar.gz"
      SHA256=e213f936af73344de071a7794233a328028045c08df58ac9c637a0e6a2ad7b3f
      SOURCE="${SOURCES}/${LIBRARY}"
      BUILD="${PREFIX}/build-${LIBRARY}"
      pushd "$SOURCES"
      #git clone -b dev-branch --single-branch --depth 1 https://github.com/UPPAALModelChecker/UUtils.git "$LIBRARY"
      [ -r "$ARCHIVE" ] || curl -sL "https://github.com/UPPAALModelChecker/UUtils/archive/refs/tags/v${VERSION}.tar.gz" -o "$ARCHIVE"
      if [ -n "$(command -v sha256sum)" ]; then echo "$SHA256 $ARCHIVE" | sha256sum --check ; fi
      [ -d "$SOURCE" ] || tar -xf "$ARCHIVE"
      popd
      echo "Building $LIBRARY in $BUILD"
      echo "  CMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE"
      echo "  CMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE"
      echo "  CMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH"
      echo "  CMAKE_INSTALL_PREFIX=$CMAKE_INSTALL_PREFIX"
      echo "  CMAKE_GENERATOR=$CMAKE_GENERATOR"
      cmake -S "$SOURCE" -B "$BUILD" -DUUtils_WITH_TESTS=OFF -DUUtils_WITH_BENCHMARKS=OFF
      cmake --build "$BUILD" --config $CMAKE_BUILD_TYPE
      cmake --install "$BUILD" --config $CMAKE_BUILD_TYPE --prefix="$CMAKE_INSTALL_PREFIX"
      rm -Rf "$BUILD"
      rm -Rf "$SOURCE"
    fi
done
