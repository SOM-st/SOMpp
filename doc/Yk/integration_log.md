# SOM++ × Yk JIT Integration: Changes and Rationale

This document records every change made to integrate SOM++ with the
[Yk meta-tracing JIT](https://github.com/ykjit/yk), explains why each change
was needed, and notes any differences from the equivalent CSOM integration.

---

## 1. Yk runtime wiring

### 1.1 `YkMT` initialisation and shutdown — `src/vm/Universe.h`, `src/vm/Universe.cpp`

**What changed.**

`Universe.h` exposes the global Yk meta-tracer handle under `USE_YK`:

```cpp
#ifdef USE_YK
#include <yk.h>
extern YkMT* global_yk_mt;
#endif
```

`Universe.cpp` defines it and allocates it at startup inside
`Universe::initialize()`, and frees it in `Universe::Shutdown()`:

```cpp
#ifdef USE_YK
YkMT* global_yk_mt = nullptr;
#endif

void Universe::initialize(...) {
    InitializeAllocationLog();
#ifdef USE_YK
    {
        char* yk_err = nullptr;
        global_yk_mt = yk_mt_new(&yk_err);
        if (yk_err != nullptr) {
            fprintf(stderr, "[YK] init failed: %s\n", yk_err);
            free(yk_err);
            global_yk_mt = nullptr;
        }
    }
#endif
    ...
}

void Universe::Shutdown() {
    ...
#ifdef USE_YK
    if (global_yk_mt != nullptr) {
        yk_mt_shutdown(global_yk_mt);
        global_yk_mt = nullptr;
    }
#endif
}
```

**Why.**
`YkMT` is Yk's meta-tracer object; every Yk-integrated program creates exactly
one at startup and destroys it on exit. It must be globally accessible because
`Universe::initialize` creates it, but the control point in `Interpreter::Start`
(a separate translation unit) reads it at every bytecode dispatch.

**Difference from CSOM.**
CSOM initialises in `Universe_start`; SOM++ uses `Universe::initialize` (called
from `Universe::Start`). Shutdown in CSOM is in `Universe_destruct`; in SOM++ it
is in `Universe::Shutdown`, which is the natural teardown path.

---

### 1.2 Per-bytecode `YkLocation` array — `src/vmobjects/VMMethod.h`, `src/vmobjects/VMMethod.cpp`

**What changed.**

`VMMethod.h` adds a `yklocs` field to the class:

```cpp
#ifdef USE_YK
    void* yklocs{nullptr};  // YkLocation[], one per bytecode
#endif
```

The destructor is moved out of the header so that `USE_YK` cleanup can be
added without pulling `<yk.h>` into every translation unit that includes
`VMMethod.h`.

`VMMethod.cpp` allocates and initialises the array at construction time, and
drops the locations in the destructor:

```cpp
#ifdef USE_YK
#include <yk.h>
#endif

VMMethod::VMMethod(...) {
    ...
#ifdef USE_YK
    if (bcCount > 0) {
        yklocs = malloc(bcCount * sizeof(YkLocation));
        if (yklocs != nullptr) {
            for (size_t i = 0; i < bcCount; i++)
                static_cast<YkLocation*>(yklocs)[i] = yk_location_new();
        }
    }
#endif
}

VMMethod::~VMMethod() {
    delete lexicalScope;
#ifdef USE_YK
    if (yklocs != nullptr) {
        for (size_t i = 0; i < bcLength; i++)
            if (!yk_location_is_null(static_cast<YkLocation*>(yklocs)[i]))
                yk_location_drop(static_cast<YkLocation*>(yklocs)[i]);
        free(yklocs);
    }
#endif
}
```

**Why.**
Yk's control point takes a `YkLocation*` that identifies the loop being traced.
Separate location objects are required for each bytecode index in each method so
the tracer can distinguish different loops. Allocating at method creation time
keeps the hot dispatch path free of allocation.

**Difference from CSOM.**
CSOM stores `yklocs` as `void*` on a C struct; SOM++ does the same (using
`void*` in the header to avoid leaking `<yk.h>` to all includers, then casting
inside `VMMethod.cpp` where `<yk.h>` is available). CSOM exposes a separate
`VMMethod_cleanup_yklocs` function; SOM++ integrates cleanup into the
destructor. `yklocs` is not tracked by the GC — it is a plain
separately-malloc'd array. When the moving GC copies a `VMMethod` object, the
`yklocs` pointer is copied as-is; the original becomes unreachable without the
destructor running, so the array remains valid for the live clone.

---

### 1.3 Control point in the dispatch loop — `src/interpreter/Interpreter.h`, `src/interpreter/Interpreter.cpp`

**What changed.**

`Interpreter.h` adds a `YK_CONTROL_POINT()` helper macro and inserts it into
both dispatch macros:

```cpp
#ifdef USE_YK
#include <yk.h>
#define YK_CONTROL_POINT()                                              \
    yk_mt_control_point(global_yk_mt,                                   \
        &static_cast<YkLocation*>(method->yklocs)[bytecodeIndexGlobal])
#else
#define YK_CONTROL_POINT() (void)0
#endif

#define DISPATCH_NOGC()                                           \
    {                                                             \
        YK_CONTROL_POINT();                                       \
        goto* loopTargets[currentBytecodes[bytecodeIndexGlobal]]; \
    }

#define DISPATCH_GC()                                             \
    {                                                             \
        YK_CONTROL_POINT();                                       \
        if (GetHeap<HEAP_CLS>()->isCollectionTriggered()) {       \
            startGC();                                            \
        }                                                         \
        goto* loopTargets[currentBytecodes[bytecodeIndexGlobal]]; \
    }
```

`Interpreter.cpp` also calls `YK_CONTROL_POINT()` at the initial dispatch
(before the first `goto*`) so the very first bytecode is covered:

```cpp
    YK_CONTROL_POINT();
    goto* loopTargets[currentBytecodes[bytecodeIndexGlobal]];
```

**Why.**
`yk_mt_control_point` tells Yk "I am at this point in the hot loop". Yk counts
how many times each `YkLocation` is reached; once the threshold is exceeded it
begins tracing.

**Difference from CSOM.**
CSOM uses a `while(true)` / `switch` loop, so a single control-point call at
the top of the while body covers every bytecode dispatch. SOM++ uses a computed
`goto*` dispatch table (threaded code) with no single loop entry point. The
control point is instead placed in the `DISPATCH_NOGC` and `DISPATCH_GC`
macros, which are called by every bytecode handler before jumping to the next
bytecode. The initial dispatch (before any handler runs) also gets the control
point so the first bytecode is not skipped.

---

## 2. Build system — `CMakeLists.txt`

### 2.1 Yk compiler and linker flags

**What changed.**

A `YK_BUILD_TYPE` CMake cache variable is added. When set to `debug` or
`release`, the build queries `yk-config` for all necessary flags and applies
them to the `SOM++` target:

```cmake
set(YK_BUILD_TYPE "" CACHE STRING
    "Yk build type (debug or release). Enables Yk JIT when set.")

if(NOT "${YK_BUILD_TYPE}" STREQUAL "")
  execute_process(COMMAND yk-config ${YK_BUILD_TYPE} --cppflags ...)
  # strip -DUSE_YK from cppflags; added explicitly via target_compile_definitions
  execute_process(COMMAND yk-config ${YK_BUILD_TYPE} --cflags ...)
  execute_process(COMMAND yk-config ${YK_BUILD_TYPE} --ldflags ...)
  execute_process(COMMAND yk-config ${YK_BUILD_TYPE} --libs ...)

  target_compile_definitions(SOM++ PRIVATE USE_YK)
  target_compile_options(SOM++ PRIVATE ${YK_CPPFLAGS_LIST} ${YK_CFLAGS_LIST})
  target_link_options(SOM++ PRIVATE ${YK_LDFLAGS_LIST})
  target_link_libraries(SOM++ ${YK_LIBS_LIST})
endif()
```

To build with Yk:

```bash
cmake -DCMAKE_CXX_COMPILER=$(yk-config debug --cc) \
      -DCMAKE_BUILD_TYPE=Release \
      -DYK_BUILD_TYPE=debug \
      -B cmake-yk .
cmake --build cmake-yk
```

**Why.**
Yk requires a patched clang (`yk-config --cc`) for LTO and its custom LLVM
passes. The flags mirror those used in CSOM's `build/unix.make` and in other
Yk-integrated interpreters (yklua, ykmicropython).

**Difference from CSOM.**
CSOM uses a GNU Make variable block guarded by
`ifneq ($(strip $(YK_BUILD_TYPE)),)`. The equivalent CMake pattern uses
`execute_process` to call `yk-config` at configure time and
`target_compile_definitions` / `target_compile_options` / `target_link_options`
to apply the results. The compiler override (`YK_CC`) must be passed as
`-DCMAKE_CXX_COMPILER=...` at configure time since CMake does not support
changing the compiler after `project()`.

---

## 3. Yk framework changes

The Yk framework changes from the CSOM integration (§4.1 promoting
internal-linkage globals in `Linkage.cpp`, §4.2 skipping
`@llvm.experimental.noalias.scope.decl` in `YkIRWriter.cpp`) live in the Yk /
LLVM tree and are not SOM++-specific. They apply equally here and are assumed
to be present in the Yk build used.
