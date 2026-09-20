#!/bin/bash

#change to base directory
cd $(dirname $0)"/.."

#delete all old binaries
rm -rf bin

make clean; make DEST_DIR=bin/generational_cache_noTagging   GC_TYPE=generational CACHE_INTEGER=true  USE_TAGGING=false install -j5
make clean; make DEST_DIR=bin/generational_cache_tagging   GC_TYPE=generational CACHE_INTEGER=true  USE_TAGGING=true install -j5
make clean; make DEST_DIR=bin/generational_nocache_noTagging GC_TYPE=generational CACHE_INTEGER=false USE_TAGGING=false install -j5
make clean; make DEST_DIR=bin/generational_nocache_tagging GC_TYPE=generational CACHE_INTEGER=false USE_TAGGING=true install -j5
make clean; make DEST_DIR=bin/generational_badcache_noTagging GC_TYPE=generational CACHE_INTEGER=true INT_CACHE_MIN_VALUE=100000 INT_CACHE_MAX_VALUE=100105 USE_TAGGING=false install -j5
make clean; make DEST_DIR=bin/generational_additional_allocation GC_TYPE=generational CACHE_INTEGER=false USE_TAGGING=true ADDITIONAL_ALLOCATION=true install -j5

make clean; make DEST_DIR=bin/copying_cache_noTagging   GC_TYPE=copying CACHE_INTEGER=true  USE_TAGGING=false install -j5
make clean; make DEST_DIR=bin/copying_cache_tagging   GC_TYPE=copying CACHE_INTEGER=true  USE_TAGGING=true install -j5
make clean; make DEST_DIR=bin/copying_nocache_noTagging GC_TYPE=copying CACHE_INTEGER=false USE_TAGGING=false install -j5
make clean; make DEST_DIR=bin/copying_nocache_tagging GC_TYPE=copying CACHE_INTEGER=false USE_TAGGING=true install -j5
make clean; make DEST_DIR=bin/copying_badcache_noTagging GC_TYPE=copying CACHE_INTEGER=true INT_CACHE_MIN_VALUE=100000 INT_CACHE_MAX_VALUE=100105 USE_TAGGING=false install -j5
make clean; make DEST_DIR=bin/copying_additional_allocation GC_TYPE=copying CACHE_INTEGER=false USE_TAGGING=true ADDITIONAL_ALLOCATION=true install -j5

make clean; make DEST_DIR=bin/mark_sweep_cache_noTagging   GC_TYPE=mark_sweep CACHE_INTEGER=true  USE_TAGGING=false install -j5
make clean; make DEST_DIR=bin/mark_sweep_cache_tagging   GC_TYPE=mark_sweep CACHE_INTEGER=true  USE_TAGGING=true install -j5
make clean; make DEST_DIR=bin/mark_sweep_nocache_noTagging GC_TYPE=mark_sweep CACHE_INTEGER=false USE_TAGGING=false install -j5
make clean; make DEST_DIR=bin/mark_sweep_nocache_tagging GC_TYPE=mark_sweep CACHE_INTEGER=false USE_TAGGING=true install -j5
make clean; make DEST_DIR=bin/mark_sweep_badcache_noTagging GC_TYPE=mark_sweep CACHE_INTEGER=true INT_CACHE_MIN_VALUE=100000 INT_CACHE_MAX_VALUE=100105 USE_TAGGING=false install -j5
make clean; make DEST_DIR=bin/mark_sweep_additional_allocation GC_TYPE=mark_sweep CACHE_INTEGER=false USE_TAGGING=true ADDITIONAL_ALLOCATION=true install -j5

make clean; make DEST_DIR=bin/cppsom_statistics GC_TYPE=generational CACHE_INTEGER=false USE_TAGGING=false LOG_RECEIVER_TYPES=true GENERATE_INTEGER_HISTOGRAM=true GENERATE_ALLOCATION_STATISTICS=true install -j5

make clean
