# UPPAAL DBM Library

See details about the UPPAAL DBM Libary at the [wiki pages](https://github.com/UPPAALModelChecker/UDBM/wiki).

## Build 
The following packages need to be installed:

  * Compiler: GCC-10 g++-10
  * Build system generator: cmake >= 3.15
  * Build driver: make (or ninja)

To Install Packages on ubuntu:
``` sh
apt install gcc-10 cmake make
``` 

Compile source:
``` sh
cmake -B build/ -DCMAKE_BUILD_TYPE=Release
cmake --build build/
```
