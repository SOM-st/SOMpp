#!/bin/bash

init_submodule_and_path() {
  git submodule update --init
  if [ "$MACHINE" = "zullie1" ]; then
    export PATH=/opt/local/bin:/Users/gitlab-runner/Library/Python/3.12/bin:$PATH
    export MP='-mp'
  fi
  export PATH=$HOME/.local/bin:$PATH
}

prepare_build_folders() {
  rm -rf release && mkdir release
  rm -rf debug && mkdir debug
}

set_compiler() {
  echo "set_compiler: COMPILER: $COMPILER, MACHINE_LOCATION: $MACHINE_LOCATION"
  if [ "$COMPILER" = "gcc" ];   then
    if [ "$MACHINE_LOCATION" = "ssw" ]; then
      # Debian Trixie's standard compiler is GCC 14, and we use libstdc++
      # from it, even for older GCCs. Probably something that could be fixed in CMake...
      export CC=gcc-14;
      export CXX=g++-14;
    else
      export CC=gcc-13;
      export CXX=g++-13;
    fi
  fi
  if [ "$COMPILER" = "clang" ]; then export CC=clang$MP-18; export CXX=clang++$MP-18; fi

  echo "set_compiler: CC: $CC, CXX: $CXX"
}

determine_name() {
  export NAME="som-$COMPILER-$GC"
  if [[ $CMAKE_FLAGS =~ "USE_TAGGING=true" ]]; then
    NAME="$NAME-inttag"
  else
    NAME="$NAME-intbox"
  fi
  
  if [[ $CMAKE_FLAGS =~ "CACHE_INTEGER=true" ]]; then
    NAME="$NAME-intcache"
  fi
  if [[ $CMAKE_FLAGS =~ "DUSE_VECTOR_PRIMITIVES=false" ]]; then
    NAME="$NAME-somvec"
  fi
  
  NAME=$( echo "$NAME" | tr '[:upper:]' '[:lower:]' )
}

make_and_test_debug_build() {
  cd debug || exit 1
  cmake .. $CMAKE_FLAGS -DGC_TYPE=$GC -DCMAKE_BUILD_TYPE=Debug
  make -j
  ./SOM++ -gc-stress -cfg -cp ../Smalltalk ../TestSuite/TestHarness.som
  ./unittests -gc-stress -cfg -cp ../Smalltalk:../TestSuite/BasicInterpreterTests ../Examples/Hello.som
  ./SOM++ -prim-hash-check -cp ../Smalltalk ../Examples/Benchmarks/BenchmarkHarness.som VectorBenchmark 1 1
  cd ..
}

make_and_test_release_build() {
  cd release || exit 1
  cmake .. $CMAKE_FLAGS -DGC_TYPE=$GC -DCMAKE_BUILD_TYPE=Release
  make -j
  ./SOM++ -cfg -cp ../Smalltalk ../TestSuite/TestHarness.som
  mv SOM++ ../$NAME
  cd ..
}

test_somsom() {
  ./$NAME -cp Smalltalk:TestSuite:core-lib/SomSom/src/compiler:core-lib/SomSom/src/interpreter:core-lib/SomSom/src/primitives:core-lib/SomSom/src/vm:core-lib/SomSom/src/vmobjects core-lib/SomSom/tests/SomSomTests.som
}

set_machine_tag_and_experiment() {
  if [ "$MACHINE" = "zullie1" ] || [ "$MACHINE" = "cassius" ]; then
    export M=''
  # for benchmarking we treat these machines like the yuria ones, just to
  # have some load balancing
  elif [ "$MACHINE" = "brutus" ]; then
    export M="t:yuria"
  elif [ "$MACHINE" = "laertes" ]; then
    export M="t:yuria2"
  elif [ "$MACHINE" = "ophelia" ]; then
    export M="t:yuria3"
  else
    export M="t:$MACHINE"
  fi
  echo "set_machine_tag_and_experiment: MACHINE: $MACHINE, M: $M"

  if [ "$MACHINE_LOCATION" = "ssw" ]; then
    if [ "$MACHINE" = "cassius" ]; then
      # cassius is comparably slow, so, use the old settings
      export EXPERIMENT="SOM++"
    else
      export EXPERIMENT="SOM++-ssw"
    fi
  else
    export EXPERIMENT="SOM++"
  fi
}
