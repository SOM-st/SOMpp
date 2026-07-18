# Run `just` for the list of recipes.

default:
    @just --list

# initialize submodules (core-lib)
submodules:
    git submodule update --init --recursive

# optimized release build in cmake-build/
build:
    mkdir -p cmake-build
    cd cmake-build && cmake -DCMAKE_BUILD_TYPE=Release .. && make

# debug build with assertions in cmake-debug/
debug:
    mkdir -p cmake-debug
    cd cmake-debug && cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j

# run the SOM test suite against the release build
test: build
    cd cmake-build && ./SOM++ -cp ../Smalltalk ../TestSuite/TestHarness.som

# run cppunit unit tests against the debug build
unittests: debug
    cd cmake-debug && ./unittests -cp ../Smalltalk:../TestSuite/BasicInterpreterTests ../Examples/Hello.som

# run the Hello World example against the release build
hello: build
    cd cmake-build && ./SOM++ -cp ../Smalltalk ../Examples/Hello.som

# lint with clang-tidy and clang-format (same checks as CI)
lint:
    clang-tidy --config-file=.clang-tidy src/**/*.cpp -- -fdiagnostics-absolute-paths
    clang-format --dry-run --style=file --Werror src/*.cpp src/**/*.cpp src/**/*.h

# remove all build dirs
clean:
    rm -rf cmake-build
    rm -rf cmake-debug