#!/usr/bin/env bash
set -euxo pipefail

# Cursed line that should also work on macos
# https://stackoverflow.com/questions/59895/how-can-i-get-the-source-directory-of-a-bash-script-from-within-the-script-itsel
SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
mkdir -p "$SOURCE_DIR/libs/sources";
CMAKE_PREFIX_PATH="$SOURCE_DIR/libs/"

CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH"
if [ -z ${CMAKE_TOOLCHAIN_FILE+x} ]; then
	echo "Not using a custom toolchain";
else
	echo "Using toolchain $CMAKE_TOOLCHAIN_FILE";
	CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE";
fi


cd $SOURCE_DIR/libs/sources;
wget https://github.com/Cyan4973/xxHash/archive/refs/tags/v0.8.0.tar.gz
tar -xvf v0.8.0.tar.gz
mkdir -p "$SOURCE_DIR/libs/sources/xxHash-0.8.0/build"
cd "$SOURCE_DIR/libs/sources/xxHash-0.8.0/build"
cmake $CMAKE_ARGS -DCMAKE_INSTALL_PREFIX="$SOURCE_DIR/libs/xxHash" -DBUILD_SHARED_LIBS=OFF ../cmake_unofficial
cmake --build . --config Release
cmake --install . --config Release

#git clone https://github.com/UPPAALModelChecker/UUtils "$SOURCE_DIR/libs/sources/UUtils";
cd $SOURCE_DIR/libs/sources;
wget https://github.com/UPPAALModelChecker/UUtils/archive/refs/tags/v1.1.1.tar.gz
tar -xvf v1.1.1.tar.gz
mkdir -p "$SOURCE_DIR/libs/sources/UUtils-1.1.1/build"
cd "$SOURCE_DIR/libs/sources/UUtils-1.1.1"
cd build
cmake $CMAKE_ARGS -DCMAKE_INSTALL_PREFIX="$SOURCE_DIR/libs/UUtils" ..
cmake --build . --config Release
cmake --install . --config Release

cd $SOURCE_DIR/libs/sources;
wget -nc https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.tar.gz
tar -xvf boost_1_81_0.tar.gz
mkdir -p "$SOURCE_DIR/libs/sources/boost_1_81_0/build"
cd "$SOURCE_DIR/libs/sources/boost_1_81_0/build"
cp -r "$SOURCE_DIR/libs/sources/boost_1_81_0/boost" "$SOURCE_DIR/libs/"
