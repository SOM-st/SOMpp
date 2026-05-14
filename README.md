SOM++ - The Simple Object Machine implemented in C++
====================================================

Introduction
------------

SOM is a minimal Smalltalk dialect used to teach VM construction at the [Hasso
Plattner Institute][SOM]. It was originally built at the University of Århus
(Denmark) where it was also used for teaching.

Implementations exist for instance for Java (SOM), C (CSOM), C++ (SOM++), Python & RPython
(PySOM), the Truffle framework (TruffleSOM), and Squeak/Pharo Smalltalk (AweSOM).

A simple SOM Hello World looks like:

```Smalltalk
Hello = (
  run = (
    'Hello World!' println.
  )
)
```

This repository contains the C++ implementation of SOM. The implementation of
the SOM standard library and a number of examples are included as a git submodule.
Please see the [main project page][SOMst] for links to other VM implementations.


Prerequisites
-------------

The SOM standard library and examples are located in the `core-lib` git submodule.
Before building, initialize and fetch the submodules:

```bash
git submodule update --init --recursive
```


Building
--------

SOM++ uses CMake and an optimized release build can be built like this:

```bash
mkdir cmake-build && cd cmake-build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

Afterwards, the tests can be executed with:

```bash
./SOM++ -cp ../Smalltalk ../TestSuite/TestHarness.som
```

A simple Hello World program is executed with:

```bash
./SOM++ -cp ../Smalltalk ../Examples/Hello.som
```

Information on previous authors are included in the AUTHORS file. This code is
distributed under the MIT License. Please see the LICENSE file for details.
Additional documentation, detailing for instance the object model and how to
implement primitives, is available in the `doc` folder.


Advanced Compilation Settings
-----------------------------

SOM++ supports different garbage collectors, including a basic mark/sweep, and
a generational GC. Furthermore, it implements different variants for integer
handling.


Tagged integers:

    default: off
    option name: TAGGING
    example: cmake .. -DUSE_TAGGING=true

Integer caching:

    default: off
    option name: INT_CACHE
    example: cmake .. -DCACHE_INTEGER=true


Development Build and Testing
-----------------------------

A build with assertions for development and debugging can be built with:

```bash
mkdir cmake-debug && cd cmake-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j
```

This build also creates a `unittests` binary, which uses cppunit for
implementation-level unit tests, to run the basic interpreter tests
in `TestSuite/BasicInterpreterTests`, as well as the SOM test suite.

The unit tests need the SOM classpath set as follows:

```bash
./unittests -cp ../Smalltalk:../TestSuite/BasicInterpreterTests ../Examples/Hello.som
```

Code Style and Linting
----------------------

To have a somewhat consistent code style and catch some basic bugs, the CI
setup runs `clang-format` and `clang-tidy`

```bash
clang-tidy --config-file=.clang-tidy src/**/*.cpp -- -fdiagnostics-absolute-paths
clang-format --dry-run --style=file --Werror src/*.cpp  src/**/*.cpp src/**/*.h
```

Build Status
------------

Thanks to GitHub Actions, this repository is automatically tested.
The current build status is: [![Build Status](https://github.com/SOM-st/SOMpp/actions/workflows/ci.yml/badge.svg)](https://github.com/SOM-st/SOMpp/actions/workflows/ci.yml)

 [SOM]: http://www.hpi.uni-potsdam.de/hirschfeld/projects/som/
 [SOMst]: https://som-st.github.io
