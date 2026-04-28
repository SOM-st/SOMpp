#pragma once

// yk.h is a C header with two C++-incompatibilities:
//   1. Uses `restrict`, which is C99 but not C++. Clang supports `__restrict`.
//   2. Missing `extern "C"` guards — without them C++ mangles the function
//      names and they won't link against the C ykcapi library.
#ifdef __cplusplus
#define restrict __restrict
extern "C" {
#endif
#include <yk.h>
#ifdef __cplusplus
#undef restrict
}
#endif
