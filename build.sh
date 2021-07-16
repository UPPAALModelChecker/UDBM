#!/usr/bin/env bash
set -e

for config in Debug Release ; do
    rm -Rf build-$config
    mkdir -p build-$config
    pushd build-$config
    cmake .. -DCMAKE_BUILD_TYPE=$config -DTESTING=ON
    cmake --build . --config $config
    ctest
    popd
done
