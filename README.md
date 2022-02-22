# UPPAAL DBM Library
UDBM is a DBM library developed for the UPPAAL project.

DBM stands for Difference Bound Matrix and used to capture and reason about clock constraints described in Timed Automata models.

The general form of the constraints is the following: `x_i - x_j <=> c_ij`, where `x_i` and `x_j` are clocks (one of them can be "zero clock" which is always zero) and `<=` can be one of {`<`, `<=`, `=`, `>=`, `>`} and `c_ij` is an integer constant.

For more details please see [wiki pages](https://github.com/UPPAALModelChecker/UDBM/wiki).

## Build 
The following packages need to be installed: 
  * cmake 
  * gcc 
  * xxHash
  * [UUtils](https://github.com/UPPAALModelChecker/UUtils)
  * doctest (test only)
  * boost (test only)

Compile source:
```sh
cmake -B build/ -DCMAKE_BUILD_TYPE=Release
cmake --build build/
```

You can also insall dependencies locally by running: 
```
./getlibs.sh 
```
