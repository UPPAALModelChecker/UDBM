#!/usr/bin/env bash
set -eo pipefail

PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

if [ $# -eq 0 ]; then
  echo "Script $0 fetches, compiles and installs dependent libraries."
  machine=$(uname --machine)
  kernel=$(uname --kernel-name)
  targets=${machine,,}-${kernel,,}
  echo "Expects targets as arguments, for example: $targets"
  echo "The following are supported:"
  for file in "$PROJECT_DIR/cmake/toolchain"/*.cmake ; do
      cmake=$(basename "$file")
      toolchain=${cmake%.cmake}
      echo -e "\t${toolchain}"
  done
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

function show_cmake_vars() {
    for var in CMAKE_TOOLCHAIN_FILE CMAKE_BUILD_TYPE CMAKE_PREFIX_PATH CMAKE_INSTALL_PREFIX \
               CMAKE_GENERATOR CMAKE_BUILD_PARALLEL_LEVEL; do
        echo "  $var=${!var:- (unset)}"
    done
}

for target in $targets ; do
    PREFIX="$PROJECT_DIR/local/$target"
    export CMAKE_TOOLCHAIN_FILE="$PROJECT_DIR/cmake/toolchain/$target.cmake"
    export CMAKE_PREFIX_PATH="$PREFIX"
    export CMAKE_INSTALL_PREFIX="$PREFIX"

    ## XXHASH for UUtils
    NAME=xxHash
    VERSION=0.8.3
    LIBRARY="${NAME}-${VERSION}"
    ARCHIVE="$LIBRARY.tgz"
    SHA256=aae608dfe8213dfd05d909a57718ef82f30722c392344583d3f39050c7f29a80
    SOURCE="${SOURCES}/$LIBRARY"
    BUILD="${PREFIX}/build-$LIBRARY"
    if [ -r "${CMAKE_INSTALL_PREFIX}/include/xxhash.h" ]; then
        echo "$LIBRARY is already installed in $CMAKE_INSTALL_PREFIX"
    else
        pushd "$SOURCES"
        [ -r "${ARCHIVE}" ] || curl -sL "https://github.com/Cyan4973/xxHash/archive/refs/tags/v$VERSION.tar.gz" -o "${ARCHIVE}"
        if [ -n "$(command -v shasum)" ]; then
            echo "$SHA256  $ARCHIVE" | shasum -a256 --check -
        fi
        [ -d "$SOURCE" ] || tar xf "${ARCHIVE}"
        popd
        echo "Building $LIBRARY in $BUILD from $SOURCE"
        show_cmake_vars
        cmake -S "$SOURCE/cmake_unofficial" -B "$BUILD" -DBUILD_SHARED_LIBS=OFF
        cmake --build "$BUILD" --config $CMAKE_BUILD_TYPE
        cmake --install "$BUILD" --config $CMAKE_BUILD_TYPE --prefix "${CMAKE_INSTALL_PREFIX}"
        rm -Rf "$BUILD"
        rm -Rf "$SOURCE"
    fi

    ## BOOST for UUtils
    NAME=boost
    VERSION=1.89.0
    LIBRARY="${NAME}-${VERSION}"
    ARCHIVE="${LIBRARY}-cmake.tar.xz"
    SHA256=67acec02d0d118b5de9eb441f5fb707b3a1cdd884be00ca24b9a73c995511f74
    SOURCE="${SOURCES}/${LIBRARY}"
    BUILD="${PREFIX}/build-${LIBRARY}"
    if [ -r "${CMAKE_INSTALL_PREFIX}/include/boost/math/distributions/arcsine.hpp" ] ; then
        echo "$LIBRARY is already installed in $CMAKE_INSTALL_PREFIX"
    else
        pushd "$SOURCES"
        [ -r "${ARCHIVE}" ] || curl -sL "https://github.com/boostorg/boost/releases/download/${LIBRARY}/${ARCHIVE}" -o "${ARCHIVE}"
        if [ -n "$(command -v shasum)" ]; then
            echo "$SHA256  $ARCHIVE" | shasum -a256 --check -
        fi
        [ -d "${SOURCE}" ] || tar xf "${ARCHIVE}"
        popd
        echo "Building $LIBRARY in $BUILD from $SOURCE"
        show_cmake_vars
        cmake -S "$SOURCE" -B "$BUILD" -DBUILD_SHARED_LIBS=OFF \
          -DBOOST_INCLUDE_LIBRARIES="headers;math" -DBOOST_ENABLE_MPI=OFF -DBOOST_ENABLE_PYTHON=OFF \
          -DBOOST_RUNTIME_LINK=static -DBUILD_TESTING=OFF -DBOOST_INSTALL_LAYOUT=system
        cmake --build "$BUILD" --config $CMAKE_BUILD_TYPE
        cmake --install "$BUILD" --config $CMAKE_BUILD_TYPE --prefix "${CMAKE_INSTALL_PREFIX}"
        rm -Rf "$BUILD"
        rm -Rf "$SOURCE"
    fi

    # doctest for unit testing
    NAME=doctest
    VERSION=2.4.12
    LIBRARY="${NAME}-${VERSION}"
    if [ -r "$PREFIX/include/doctest/doctest.h" ] ; then
      echo "$LIBRARY is already installed in $PREFIX"
    else
      ARCHIVE="${LIBRARY}.tar.gz"
      SHA256=73381c7aa4dee704bd935609668cf41880ea7f19fa0504a200e13b74999c2d70
      SOURCE="${SOURCES}/${LIBRARY}"
      BUILD="${PREFIX}/build-${LIBRARY}"
      pushd "$SOURCES"
      [ -r "$ARCHIVE" ] || curl -sL "https://github.com/doctest/doctest/archive/refs/tags/v${VERSION}.tar.gz" -o "$ARCHIVE"
      if [ -n "$(command -v sha256sum)" ]; then echo "$SHA256 $ARCHIVE" | sha256sum --check ; fi
      [ -d "$SOURCE" ] || tar -xf "$ARCHIVE"
      popd
      echo "Building $LIBRARY in $BUILD"
      show_cmake_vars
      cmake -S "$SOURCE" -B "$BUILD" -DDOCTEST_WITH_TESTS=OFF \
           -DDOCTEST_WITH_MAIN_IN_STATIC_LIB=ON -DDOCTEST_USE_STD_HEADERS=OFF
      cmake --build "$BUILD" --config $CMAKE_BUILD_TYPE
      cmake --install "$BUILD" --config $CMAKE_BUILD_TYPE --prefix "$CMAKE_INSTALL_PREFIX"
      rm -Rf "$BUILD"
      rm -Rf "$SOURCE"
    fi

    # UUtils various low level Uppaal utilities
    NAME=UUtils
    VERSION=2.0.7
    LIBRARY="${NAME}-${VERSION}"
    if [ -r "$PREFIX/include/base/Enumerator.h" ]; then
      echo "$LIBRARY is already installed in $PREFIX"
    else
      ARCHIVE="${LIBRARY}.tar.gz"
      SHA256=52a823768398707eee42392182c320141662e74e6bd8eaac1a0eca22d8a27bcd
      SOURCE="${SOURCES}/${LIBRARY}"
      BUILD="${PREFIX}/build-${LIBRARY}"
      pushd "$SOURCES"
      #git clone -b dev-branch --single-branch --depth 1 https://github.com/UPPAALModelChecker/UUtils.git "$LIBRARY"
      [ -r "$ARCHIVE" ] || curl -sL "https://github.com/UPPAALModelChecker/UUtils/archive/refs/tags/v${VERSION}.tar.gz" -o "$ARCHIVE"
      if [ -n "$(command -v sha256sum)" ]; then echo "$SHA256 $ARCHIVE" | sha256sum --check ; fi
      [ -d "$SOURCE" ] || tar -xf "$ARCHIVE"
      popd
      echo "Building $LIBRARY in $BUILD"
      show_cmake_vars
      cmake -S "$SOURCE" -B "$BUILD" -DUUtils_WITH_TESTS=OFF -DUUtils_WITH_BENCHMARKS=OFF
      cmake --build "$BUILD" --config $CMAKE_BUILD_TYPE
      cmake --install "$BUILD" --config $CMAKE_BUILD_TYPE --prefix="$CMAKE_INSTALL_PREFIX"
      rm -Rf "$BUILD"
      rm -Rf "$SOURCE"
    fi
done
