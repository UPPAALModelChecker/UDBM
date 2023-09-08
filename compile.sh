#!/usr/bin/env bash
set -eo pipefail

PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

if [ $# -eq 0 ]; then
    targets="x86_64-linux-ubsan-asan-debug x86_64-linux-release x86_64-w64-mingw32-debug x86_64-w64-mingw32-release"
else
    targets="$@"
fi

for target in $targets ; do
    unset BUILD_EXTRA
    unset CMAKE_BUILD_TYPE
    case $target in
        x86_64-linux-*)
            PLATFORM=x86_64-linux
            ;;
        i686-linux-*)
            PLATFORM=i686-linux
            ;;
        x86_64-w64-mingw32-*)
            PLATFORM=x86_64-w64-mingw32
            ;;
        i686-w64-mingw32-*)
            PLATFORM=i686-w64-mingw32
            ;;
        *)
            echo "Unrecognized target platform: $target"
            exit 1
    esac
    export CMAKE_TOOLCHAIN_FILE="$PROJECT_DIR/cmake/toolchain/$PLATFORM.cmake"
    export CMAKE_PREFIX_PATH="$PROJECT_DIR/local/$PLATFORM"
    BUILD_DIR="build-$PLATFORM"

    case $target in
        *-ubsan-*)
            BUILD_EXTRA="$BUILD_EXTRA -DUBSAN=ON"
            BUILD_DIR="${BUILD_DIR}-ubsan"
            ;;
    esac
    case $target in
        *-lsan-*)
            BUILD_EXTRA="$BUILD_EXTRA -DLSAN=ON"
            BUILD_DIR="${BUILD_DIR}-lsan"
            ;;
    esac
    case $target in
        *-asan-*)
            BUILD_EXTRA="$BUILD_EXTRA -DASAN=ON"
            BUILD_DIR="${BUILD_DIR}-asan"
            ;;
    esac
    case $target in
        *-debug)
            export CMAKE_BUILD_TYPE=Debug
            BUILD_DIR="${BUILD_DIR}-debug"
            ;;
        *-release)
            export CMAKE_BUILD_TYPE=Release
            BUILD_DIR="${BUILD_DIR}-release"
            ;;
        *)
            export CMAKE_BUILD_TYPE=Release
            BUILD_DIR="${BUILD_DIR}-release"
            echo "Unrecognized build type: $target, assuming $CMAKE_BUILD_TYPE"
    esac
    echo "Building $target with $BUILD_EXTRA into $BUILD_DIR"
    echo "  CMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE"
    echo "  CMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH"
    cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" $BUILD_EXTRA
    cmake --build "$BUILD_DIR" --config $CMAKE_BUILD_TYPE
    (cd "$BUILD_DIR" ; ctest -C $CMAKE_BUILD_TYPE --output-on-failure)
done
