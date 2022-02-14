#!/usr/bin/env bash

SOURCE_DIR=`dirname "$(readlink -f "$0")"`
mkdir -p $SOURCE_DIR/libs/sources;

git clone git@github.com:UPPAALModelChecker/UUtils.git $SOURCE_DIR/libs/sources/UUtils;
mkdir -p $SOURCE_DIR/libs/sources/UUtils/build
cd $SOURCE_DIR/libs/sources/UUtils/build
cmake -DCMAKE_INSTALL_PREFIX=$SOURCE_DIR/libs/UUtils ..
make -j $(nproc) install
