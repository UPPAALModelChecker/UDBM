#! /usr/bin/env bash

cd lib || { echo "Directory lib not found!"; exit 1; }

# udbm

mkdir tmp-lib
cd tmp-lib
for l in ../lib{base,dbm,debug,hash}.a; do
  ar x $l
done
cd ..
ar rucs libudbm.a tmp-lib/*.o
rm -rf tmp-lib
