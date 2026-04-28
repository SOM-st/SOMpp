yk_config := "/home/pd/yk-csom/bin/yk-config"

build: build-release

build-release:
    mkdir -p cmake-build
    cmake -DCMAKE_BUILD_TYPE=Release \
        "-DCMAKE_CXX_FLAGS=-I$HOME/.local/include" \
        "-DLIB_CPPUNIT=$HOME/.local/lib/libcppunit.so" \
        -S . -B cmake-build
    cmake --build cmake-build --parallel

build-debug:
    mkdir -p cmake-debug
    cmake -DCMAKE_BUILD_TYPE=Debug \
        "-DCMAKE_CXX_FLAGS=-I$HOME/.local/include" \
        "-DLIB_CPPUNIT=$HOME/.local/lib/libcppunit.so" \
        -S . -B cmake-debug
    cmake --build cmake-debug --parallel

build-yk-debug:
    mkdir -p cmake-yk
    PATH="$(dirname {{yk_config}}):$PATH" cmake \
        -DCMAKE_CXX_COMPILER=$({{yk_config}} debug --cc)++ \
        -DCMAKE_BUILD_TYPE=Debug \
        -DYK_BUILD_TYPE=debug \
        "-DCMAKE_CXX_FLAGS=-I$HOME/.local/include" \
        "-DLIB_CPPUNIT=$HOME/.local/lib/libcppunit.so" \
        -S . -B cmake-yk
    cmake --build cmake-yk --parallel

build-yk-release:
    mkdir -p cmake-yk
    PATH="$(dirname {{yk_config}}):$PATH" cmake \
        -DCMAKE_CXX_COMPILER=$({{yk_config}} debug --cc)++ \
        -DCMAKE_BUILD_TYPE=Release \
        -DYK_BUILD_TYPE=release \
        -S . -B cmake-yk
    cmake --build cmake-yk --parallel

build-yk: build-yk-debug

test: test-unit test-som

test-som: build-release
    cmake-build/SOM++ -cp Smalltalk TestSuite/TestHarness.som

test-unit: build-debug
    cmake-debug/unittests -cp Smalltalk:TestSuite/BasicInterpreterTests Examples/Hello.som

test-yk: build-yk
    cmake-yk/SOM++ -cp Smalltalk TestSuite/TestHarness.som

hello: build-release
    cmake-build/SOM++ -cp Smalltalk Examples/Hello.som


hello-yk: build-yk-debug
    cmake-yk/SOM++ -cp Smalltalk Examples/Hello.som

awfy: build-release
    #!/usr/bin/env bash
    for b in Ball Bounce List Mandelbrot Permute Queens Sieve Storage Towers; do
        cmake-build/SOM++ -cp Smalltalk:Examples/AreWeFastYet Examples/AreWeFastYet/Harness.som $b 200 10 2>/dev/null | grep average
    done

awfy-compare: build-release build-yk-release
    #!/usr/bin/env bash
    for b in Ball Bounce List Mandelbrot Permute Queens Sieve Storage Towers; do
        echo "=== $b ==="
        printf "plain: "; cmake-build/SOM++ -cp Smalltalk:Examples/AreWeFastYet Examples/AreWeFastYet/Harness.som $b 200 10 2>/dev/null | grep average
        printf "yk:    "; cmake-yk/SOM++    -cp Smalltalk:Examples/AreWeFastYet Examples/AreWeFastYet/Harness.som $b 200 10 2>/dev/null | grep average
    done

clean:
    rm -rf cmake-build cmake-debug cmake-yk
