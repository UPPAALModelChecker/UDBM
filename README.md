# UPPAAL DBM Library
<acronym title="Difference Bound Matrix">DBM</acronym>s [[dill89, rokicki93, lpw:fct95, bengtsson02]](#-References) are efficient data structures to represent clock constraints in timed automata [[ad90]](#-References).
They are used in UPPAAL [[lpy97, by04, bdl04]](#-References) as the core data structure to represent time.
The library features all the common operations such as up (delay, or future), down (past), general updates, different extrapolation functions, etc. on DBMs and federations.
The library also supports subtractions.
The API is in C and C++. The C++ part uses active clocks and hides (to some extent) memory management.

  * Reliable <br/>
    The DBM library has an extensive test suite with an extra alternative implementation of the algorithms. This implementation has also been tested on countless case studies.
  * Performant <br/>
    The DBM library is the fruit of many years of development of UPPAAL, bringing to the mainstream efficient algorithms to manipulate DBMs. The API is available in C and C++.
  * Current <br/>
    The DBM library is based on the latest internal development version of UPPAAL, containing the latest performance and language improvements.

The general form of the constraints is the following: `x_i - x_j <=> c_ij`, where `x_i` and `x_j` are clocks (one of them can be "zero clock" which is always zero) and `<=` can be one of {`<`, `<=`, `=`, `>=`, `>`} and `c_ij` is an integer constant.

For more details please see [wiki pages](https://github.com/UPPAALModelChecker/UDBM/wiki).

## Build
The following packages need to be installed:
  * [bash](https://www.gnu.org/software/bash/)
  * [cmake](https://cmake.org/)
  * [GCC](https://gcc.gnu.org/), [clang](https://clang.llvm.org/) or other C++17 compiler.
  * [xxHash](https://github.com/Cyan4973/xxHash)
  * [UUtils](https://github.com/UPPAALModelChecker/UUtils)
  * [doctest](https://github.com/doctest/doctest) (just for unit tests)

The dependencies can be installed locally into `local` directory by running `getlibs.sh`:
```shell
./getlibs.sh
```

Compile source with **release** optimizations:
```shell
cmake -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$PWD/local
cmake --build build-release
```

Compile source with **release** optimizations and **unit tests**:
```shell
cmake -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$PWD/local -DTESTING=ON
cmake --build build-release
(cd build-release/test ; ctest)
```

Compile source with **debug**, **sanitizers** and **unit tests**:
```shell
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=$PWD/local -DTESTING=ON -DSSP=ON -DUBSAN=ON -DASAN=ON
cmake --build build-debug
(cd build-debug/test ; ctest)
```

## References

* [[dill89]](https://doi.org/10.1007/3-540-52148-8_17) David L. Dill. _Timing Assumptions and Verification of Finite-State Concurrent Systems._ LNCS 407\. Springer Berlin 1989, pp 197-212.
* [rokicki93] Tomas Gerhard Rokicki. _Representing and Modeling Digital Circuits._ Ph.D. thesis, Standford University 1993.
* [[lpw:fct95]](https://doi.org/10.1007/3-540-60249-6_41) Kim G. Larsen, Paul Pettersson, and Wang Yi. _Model-Checking for Real-Time Systems._ Fundamentals of Computation Theory 1995, LNCS 965 pages 62-88.
* [[bengtsson02]](http://uu.diva-portal.org/smash/record.jsf?pid=diva2:161779) Johan Bengtsson. _Clocks, DBM, and States in Timed Systems._ Ph.D. thesis, Uppsala University 2002.
* [[ad90]](https://doi.org/10.1007/BFb0032042) Rajeev Alur and David L. Dill. _Automata for Modeling Real-Time Systems._ International Colloquium on Algorithms, Languages, and Programming 1990, LNCS 443 pages 322-335.
* [[lpy97]](https://doi.org/10.1007/s100090050010) Kim G. Larsen, Paul Pettersson, and Wang Yi. _UPPAAL in a Nutshell._ International Journal on Software Tools for Technology Transfer , October 1997, number 1-2 pages 134-152.
* [[by04]](https://doi.org/10.1007/978-3-540-27755-2_3) Johan Bengtsson and Wang Yi. [_Timed Automata: Semantics, Algorithms and Tools._](https://homes.cs.aau.dk/~adavid/UDBM/materials/by04-bookchapter.pdf) Concurrency and Petri Nets 2004, LNCS 3098.
* [[bdl04]](https://www.it.uu.se/research/group/darts/papers/texts/new-tutorial.pdf) Gerd Behrmann, Alexandre David, and Kim G. Larsen. _A Tutorial on UPPAAL._ 4th International School on Formal Methods for the Design of Computer, Communication, and Software Systems (SFM-RT'04), LNCS 3185.
