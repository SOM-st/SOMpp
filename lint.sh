#!/bin/sh
clang-format-mp-19 -i --style=file --Werror src/*.cpp  src/**/*.cpp src/**/*.h
clang-tidy-mp-19 --config-file=.clang-tidy src/*.cpp src/**/*.cpp \
  --fix \
  -- -I/opt/local/include/ -fdiagnostics-absolute-paths \
  -DGC_TYPE=GENERATIONAL -DUSE_TAGGING=false -DCACHE_INTEGER=false -DUNITTESTS
clang-format-mp-19 -i --style=file --Werror src/*.cpp  src/**/*.cpp src/**/*.h
