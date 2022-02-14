# UPPAAL DBM Library
UDBM is a DBM library developed in UPPAAL project.

DBM stands for Difference Bound Matrix and used to capture and reason about clock constraints described in Timed Automata models.

The general form of the constraints is the following: `x_i - x_j <=> c_ij`, where `x_i` and `x_j` are clocks (one of them can be "zero clock" which is always zero) and `<=` can be one of {`<`, `<=`, `=`, `>=`, `>`} and `c_ij` is an integer constant.

For more details please see [wiki pages](https://github.com/UPPAALModelChecker/UDBM/wiki).

## Build 
The following packages need to be installed: `cmake gcc xxHash doctest boost`.
Additionally, you need [UUtils](https://github.com/UPPAALModelChecker/UUtils), which you can install system-wide, or locally by using the script `getlibs.sh`.

Compile source:
```sh
./getlibs.sh # If not installed systemwide 
cmake -B build/ -DCMAKE_BUILD_TYPE=Release
cmake --build build/
```
