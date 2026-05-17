#pragma once

/*
 *
 *
 Copyright (c) 2007 Michael Haupt, Tobias Pape, Arne Bergmann
 Software Architecture Group, Hasso Plattner Institute, Potsdam, Germany
 http://www.hpi.uni-potsdam.de/swa/

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include <cstdint>
#include <cstdio>
#include <string>

// Macro Debugging
#define VALUE_TO_STRING(x) #x
#define VALUE(x) VALUE_TO_STRING(x)
#define VAR_NAME_VALUE(var) #var "=" VALUE(var)

//
// error codes
//
#define ERR_SUCCESS 0x0
#define ERR_FAIL 0x1
#define ERR_NOMEM 0x2
#define ERR_PANIC 0xFFFF

//
// Integer Ranges
//
#ifndef INT32_MAX
  /*Maximum value of 32-bit integer is 0x7FFF FFFF (2 147 483 647)         */
  #define INT32_MAX 0x7FFFFFFF /*2 147 483 647*/
#endif
#ifndef INT32_MIN
  #define INT32_MIN (-2147483647)

#endif

//
// defs for classpath extraction
//
#define pathSeparator ':'
#define fileSeparator '/'

//
// GC Types
//
#define GENERATIONAL 1
#define COPYING 2
#define MARK_SWEEP 3
#define DEBUG_COPYING 4

#if GC_TYPE == GENERATIONAL
class GenerationalHeap;
typedef GenerationalHeap HEAP_CLS;
  #define write_barrier(obj, value_ptr) \
      ((GetHeap<GenerationalHeap>())->writeBarrier(obj, value_ptr))
  #define ALLOC_MATURE , true
  #define ALLOC_OUTSIDE_NURSERY(X) , (X)
  #define ALLOC_OUTSIDE_NURSERY_DECL , bool outsideNursery = false
#elif GC_TYPE == COPYING
class CopyingHeap;
typedef CopyingHeap HEAP_CLS;
  #define write_barrier(obj, value_ptr)
  #define ALLOC_MATURE
  #define ALLOC_OUTSIDE_NURSERY(X)
  #define ALLOC_OUTSIDE_NURSERY_DECL
#elif GC_TYPE == MARK_SWEEP
class MarkSweepHeap;
typedef MarkSweepHeap HEAP_CLS;
  #define write_barrier(obj, value_ptr)
  #define ALLOC_MATURE
  #define ALLOC_OUTSIDE_NURSERY(X)
  #define ALLOC_OUTSIDE_NURSERY_DECL
#elif GC_TYPE == DEBUG_COPYING
class DebugCopyingHeap;
typedef DebugCopyingHeap HEAP_CLS;
  #define write_barrier(obj, value_ptr)
  #define ALLOC_MATURE
  #define ALLOC_OUTSIDE_NURSERY(X)
  #define ALLOC_OUTSIDE_NURSERY_DECL
#endif

//
// Integer Settings
//
#ifndef USE_TAGGING
  #define USE_TAGGING false
#endif

#ifdef CACHE_INTEGER
  // Sanity check
  #if CACHE_INTEGER && USE_TAGGING
    #error Caching of small integer instances is only useful without tagged integers
  #endif
#endif

#ifndef CACHE_INTEGER
  #define CACHE_INTEGER false
#endif

#ifndef INT_CACHE_MIN_VALUE
  #define INT_CACHE_MIN_VALUE (-5)
#endif

#ifndef INT_CACHE_MAX_VALUE
  #define INT_CACHE_MAX_VALUE (100)
#endif

//
// Vector Settings
//
#ifndef USE_VECTOR_PRIMITIVES
  #define USE_VECTOR_PRIMITIVES true
#endif

//
// Debugging
//
#ifdef DEBUG
  // Sanity check
  #if (!(DEBUG + 0))
    #pragma message(VAR_NAME_VALUE(DEBUG))
    #error DEBUG needs to be set to a boolean
  #endif
#endif

#ifndef NDEBUG
  #ifndef DEBUG
    #define DEBUG true
  #endif
#else
  #ifndef DEBUG
    #define DEBUG false
  #endif
#endif

//
// Testing
//
#ifndef UNITTESTS
  #define make_testable(label)
#else
  #define make_testable(label) \
  label:
#endif

//
// Log Levels
//
#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARN 1
#define LOG_LEVEL_LOG 2
#define LOG_LEVEL_INFO 3

#ifndef LOG_LEVEL
  #define LOG_LEVEL LOG_LEVEL_ERROR
#endif

//
// Performance Optimization
//
#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#if defined(_MSC_VER)
typedef unsigned long long uint64_t;
typedef long long int64_t;
typedef unsigned long uint32_t;
typedef long int32_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned char uint8_t;
typedef char int8_t;
#endif
