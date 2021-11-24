# UPPAAL DBM Library

UDBM is a DBM library developed in UPPAAL project.

DBM stands for Difference Bound Matrix and used to capture and reason about clock constraints described in Timed Automata models.

The general form of the constraints is the following: `x_i - x_j <=> c_ij`, where `x_i` and `x_j` are clocks (one of them can be "zero clock" which is always zero) and `<=` can be one of {`<`, `<=`, `=`, `>=`, `>`} and `c_ij` is an integer constant.

For more details please see [wiki pages](https://github.com/UPPAALModelChecker/UDBM/wiki).

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
