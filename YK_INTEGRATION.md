# Integrating a C/C++ Interpreter with Yk

This document records every issue encountered and how it was fixed when integrating
SOM++ (a C++ Smalltalk VM) with [Yk](https://github.com/ykjit/yk), a meta-tracing
JIT for bytecode interpreters.  It is written for the next engineer who wants to
do the same with a different interpreter.

---

## Overview

Yk instruments the interpreter binary with a set of LLVM LTO passes and a Rust
runtime (`libykrt`).  The interpreter must:

1. Call `yk_mt_control_point(mt, &location[pc])` once per bytecode dispatch.
2. Have exactly one call site for `yk_mt_control_point` in the binary.
3. Store one `YkLocation` per bytecode slot, initialized with `yk_location_new()`.

---

## Step-by-step integration guide

### 1  Build system

Install `yk-config` (provided by yk-csom) and use it to obtain the compiler and
linker flags:

```cmake
execute_process(COMMAND yk-config debug --cc OUTPUT_VARIABLE YK_CC ...)
set(CMAKE_CXX_COMPILER "${YK_CC}++")
```

Pass the Yk link flags at link time (they are LTO passes, not compile-time flags):

```cmake
execute_process(COMMAND yk-config debug --ldflags OUTPUT_VARIABLE YK_LDFLAGS ...)
target_link_options(myvm PRIVATE ${YK_LDFLAGS})
```

The `-yk-patch-control-point` and `-yk-no-vectorize` warnings emitted by clang
during *compilation* are harmless — those flags are linker-time flags and are
ignored at compile time.

### 2  Exactly one control point

Yk requires **one** call site for `yk_mt_control_point` in the entire binary.  In a
dispatch-loop interpreter this usually falls out naturally.  In a computed-goto
interpreter, every `DISPATCH` macro needs to jump to a single trampoline label
that contains the one call:

```cpp
// In interpreter header
#define DISPATCH() goto YK_DISPATCH_START

// In interpreter Start():
YK_DISPATCH_START:
    yk_mt_control_point(global_yk_mt,
        &static_cast<YkLocation*>(method->yklocs)[bytecodeIndexGlobal]);
    goto* loopTargets[currentBytecodes[bytecodeIndexGlobal]];
```

If the interpreter `Start()` function was templated (e.g.
`template<bool PrintBytecodes>`), **de-templatize it first**.  Each template
instantiation is a separate function and therefore produces a separate call site,
triggering the "Unexpected number of control point calls: 2 expected: 1" panic.

Replace `if constexpr` with ordinary `if`, and change callers from
`Interpreter::Start<flag>()` to `Interpreter::Start(flag)`.

### 3  `YkLocation` array per method

Allocate and initialize one `YkLocation` per bytecode slot when a method object
is created:

```cpp
yklocs = malloc(bcCount * sizeof(YkLocation));
for (size_t i = 0; i < bcCount; i++)
    static_cast<YkLocation*>(yklocs)[i] = yk_location_new();
```

Free them in the destructor:

```cpp
for (size_t i = 0; i < bcLength; i++)
    if (!yk_location_is_null(static_cast<YkLocation*>(yklocs)[i]))
        yk_location_drop(static_cast<YkLocation*>(yklocs)[i]);
free(yklocs);
```

**Critical layout bug (C++):** If the method object uses an inline-storage trick
where constants and bytecodes are stored immediately after the struct in the same
allocation, adding the `yklocs` pointer field shifts the inline data offset.

**Do not** compute the inline-data start by counting trailing pointer fields:
```cpp
// WRONG when yklocs is added as a third trailing pointer:
indexableFields = (gc_oop_t*)(&indexableFields + 2);
bytecodes       = (uint8_t*)(&indexableFields  + 2 + nConstants);
```

**Use `this + 1` instead**, which always points one struct past the end of the
current object regardless of how many pointer-sized fields the struct contains:

```cpp
indexableFields = reinterpret_cast<gc_oop_t*>(this + 1);
bytecodes       = reinterpret_cast<uint8_t*>(indexableFields + numberOfConstants);
```

Apply the same fix to any `CloneForMovingGC` / copy constructor that rebuilds
these pointers.

### 4  C++ global constructors and the shadow stack

Yk's shadow stack pass runs before the `--yk-outline-untraceable` pass.  Functions
that are called by C++ global constructors (which run **before `main()`**) will be
instrumented by the shadow stack pass but will execute before `main()` initialises
the shadow stack global (`shadowstack_0 = malloc(...)`).  The result is a SIGSEGV
or memory corruption.

**Fix A — avoid static-storage `std::string` members.**  Every
`static const std::string` member of a class with static storage duration triggers
a global constructor that calls into instrumented code.  Replace them with
`static constexpr const char*`.  This alone eliminates many global constructor
crashes.

### 5  LTO incompatibility with `yk-config release`


`yk-config release --cflags` injects `-flto`, which causes every `.cpp` file to be
compiled to LLVM bitcode rather than native object files.  The Yk-modified lld then
runs LTO with `-O3`, applying cross-module inlining before the Yk `YkIRWriter` pass
serialises the IR.

**The crash:** `std::out_of_range: map::at` in `YkIRWriter::serialiseFunc` at
`BBCache.at(IB)`.  The root cause is that cross-module inlining can produce PHI
nodes whose incoming basic blocks come from inlined callee bodies.  Those blocks
were never registered in `BBCache` (which only tracks blocks following the
Yk-required `TracingCheck → Record → Serialise` three-block pattern), so the lookup
throws.

**The fix in `CMakeLists.txt`** (yksompp, no ykllvm changes needed):

```cmake
# Strip -flto injected by yk-config to avoid the YkIRWriter BBCache crash.
string(REGEX REPLACE " *-flto" "" YK_CFLAGS "${YK_CFLAGS}")
```

And for the Release cmake target, skip `-O3 -flto` when a Yk build type is set:

```cmake
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if("${YK_BUILD_TYPE}" STREQUAL "")
    target_compile_options(SOM++ PRIVATE -O3 -flto)
    target_link_options(SOM++ PRIVATE -flto)
  else()
    # LTO+O3 crashes YkIRWriter. Use O2 without LTO for Yk release builds.
    target_compile_options(SOM++ PRIVATE -O2)
  endif()
endif()
```

This produces native `.o` objects that lld links without LTO, avoiding the
`BBCache` lookup entirely.  The Yk linker passes (`--yk-shadow-stack`,
`--yk-embed-ir`, etc.) still run at link time through `-mllvm=` flags and are
unaffected.

**Note:** until the upstream `YkIRWriter` is fixed, a Yk release build uses `-O2`
rather than `-O3`.  The performance gap is small compared with the JIT speedup once
traces compile successfully.

### 6  `indirectbr` and computed-goto dispatch

Yk's tracer does not support the LLVM `indirectbr` instruction.  Computed gotos
(`goto* table[opcode]`) compile to `indirectbr` in LLVM IR, so any interpreter
dispatch loop that uses them will cause every trace to abort immediately after the
control point.

**Symptom:** all `YKD_LOG_STATS` counters are zero despite the interpreter running:
```json
{ "traces_recorded_ok": 0, "traces_recorded_err": 0, "traces_compiled_ok": 0, ... }
```

**Root cause:** the control point is reached and tracing begins, but the very next
instruction is `indirectbr`.  Yk aborts the recording before a single instruction
is captured, so no trace ever completes.

**Workaround — switch-based dispatch for `USE_YK` builds:**

Replace the computed-goto dispatch with a `switch` statement when `USE_YK` is
defined:

```cpp
#ifdef USE_YK
  switch (currentBytecodes[bytecodeIndexGlobal]) {
    case BC_PUSH_LOCAL:    goto op_push_local;
    case BC_PUSH_ARGUMENT: goto op_push_argument;
    // ...
  }
#else
  goto* loopTargets[currentBytecodes[bytecodeIndexGlobal]];
#endif
```

A `switch` compiles to `br` with multiple successors (not `indirectbr`), which Yk
can trace through.  This is the recommended approach and avoids any changes to the
ykllvm fork.

**Alternative:** implement `indirectbr` support in Yk's tracer (upstream work).

---

## Diagnostic tips

- **"Unexpected number of control point calls: N expected: 1"** — you have multiple
  call sites, most likely from template instantiation.  De-templatize.
- **SIGSEGV in `__cxx_global_var_init`** — a global constructor calls shadow-stack-
  instrumented code before `main()` initializes the stack.  Check for
  `static const std::string` members; replace with `constexpr const char*`.
- **SIGSEGV in constructor (e.g. `PrimitiveLoader`)** — same root cause as above;
  eliminate remaining `static const std::string` members.
- **`std::out_of_range: map::at`** in `YkIRWriter` at link time —
  LTO cross-module inlining broke the Yk IR structure.
  Strip `-flto` from `yk-config --cflags` in `CMakeLists.txt` (see §5).
- **YkLocation state corruption (`state == 2`)** — the `yklocs` pointer field is
  being overwritten by inline-constant storage.  Apply the `this + 1` fix.
- **All `YKD_LOG_STATS` counters are zero** — tracing is aborting immediately.
  Most likely cause: computed-goto dispatch.  Switch to `switch`-based dispatch
  under `#ifdef USE_YK` (see §6).

---

## Runtime verification

```sh
# Non-Yk build should pass all tests unaffected:
just test-som

# Yk build should print "Hello, World from SOM":
just hello-yk
```
