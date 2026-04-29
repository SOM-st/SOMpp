# Integrating a C/C++ Interpreter with Yk

This document records what is required to integrate a C/C++ bytecode interpreter
with [Yk](https://github.com/ykjit/yk), a meta-tracing JIT.  It is written for
the next engineer who wants to do the same with a different interpreter.

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

`yk-config` (provided by yk-csom) supplies the compiler and linker flags.  Use
the Yk-modified clang as the C++ compiler and pass all flags through CMake:

```cmake
execute_process(COMMAND yk-config ${YK_BUILD_TYPE} --cc OUTPUT_VARIABLE YK_CC ...)
set(CMAKE_CXX_COMPILER "${YK_CC}++")

execute_process(COMMAND yk-config ${YK_BUILD_TYPE} --cppflags OUTPUT_VARIABLE YK_CPPFLAGS ...)
execute_process(COMMAND yk-config ${YK_BUILD_TYPE} --cflags   OUTPUT_VARIABLE YK_CFLAGS ...)
execute_process(COMMAND yk-config ${YK_BUILD_TYPE} --ldflags  OUTPUT_VARIABLE YK_LDFLAGS ...)
execute_process(COMMAND yk-config ${YK_BUILD_TYPE} --libs     OUTPUT_VARIABLE YK_LIBS ...)

target_compile_definitions(SOM++ PRIVATE USE_YK)
target_compile_options(SOM++ PRIVATE ${YK_CPPFLAGS_LIST} ${YK_CFLAGS_LIST} -fno-exceptions -fno-jump-tables)
target_link_options(SOM++ PRIVATE ${YK_LDFLAGS_LIST})
target_link_libraries(SOM++ ${YK_LIBS_LIST})
```

`--cppflags` provides the ykcapi include path.  `--cflags` injects `-flto` — do
**not** strip it: the Yk linker passes (`--yk-embed-ir`, `--yk-patch-control-point`,
etc.) operate on LLVM bitcode produced by LTO; without it they silently no-op and
tracing never fires.  `USE_YK` is added via `target_compile_definitions` rather than
taken from `--cppflags` so it is scoped to the target.

Add `-fno-jump-tables` so the compiler does not lower `switch` statements into
indirect-jump tables (which produce untraceable `indirectbr` IR — see §4).

The `-yk-patch-control-point` and `-yk-no-vectorize` warnings emitted by clang
during *compilation* are harmless — those are linker-time flags ignored at compile
time.

### 2  Exactly one control point

Yk requires **one** call site for `yk_mt_control_point` in the entire binary.  In a
computed-goto interpreter, every `DISPATCH` macro must jump to a single trampoline
label that contains the one call.  The trampoline then dispatches to the handler via
a `switch` (not `goto*` — see §4):

```cpp
// In interpreter header (USE_YK build)
#define DISPATCH_NOGC() goto YK_DISPATCH_START

// In interpreter Start():
YK_DISPATCH_START:
    yk_mt_control_point(global_yk_mt,
        &static_cast<YkLocation*>(method->yklocs)[bytecodeIndexGlobal]);
    switch (currentBytecodes[bytecodeIndexGlobal]) {
        case BC_PUSH_LOCAL: goto LABEL_BC_PUSH_LOCAL;
        // ... one case per opcode
    }
```

If the interpreter `Start()` function is templated (e.g.
`template<bool PrintBytecodes>`), **de-templatize it first** — each instantiation
is a separate function with its own call site.  Replace `if constexpr` with
ordinary `if` and change callers from `Interpreter::Start<flag>()` to
`Interpreter::Start(flag)`.

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

**Inline-storage layout (C++):** If the method object stores constants and bytecodes
immediately after the struct in the same allocation, adding the `yklocs` pointer
field shifts the inline data offset.  Do not count trailing pointer fields manually:

```cpp
// WRONG when yklocs is added as a third trailing pointer:
indexableFields = (gc_oop_t*)(&indexableFields + 2);
bytecodes       = (uint8_t*)(&indexableFields  + 2 + nConstants);
```

Use `this + 1` instead, which is always correct regardless of field count:

```cpp
indexableFields = reinterpret_cast<gc_oop_t*>(this + 1);
bytecodes       = reinterpret_cast<uint8_t*>(indexableFields + numberOfConstants);
```

Apply the same fix to any `CloneForMovingGC` / copy constructor that rebuilds
these pointers.

### 4  Switch-based dispatch (not computed goto)

Yk's tracer cannot trace LLVM `indirectbr` instructions.  Computed gotos
(`goto* table[opcode]`) compile to `indirectbr`, so every trace aborts immediately
after the control point — all `YKD_LOG_STATS` counters will be zero.

Replace the computed-goto dispatch with a `switch` under `#ifdef USE_YK`.  A
`switch` compiles to `br` with multiple successors, which Yk can trace through:

```cpp
#ifdef USE_YK
  switch (currentBytecodes[bytecodeIndexGlobal]) {
    case BC_PUSH_LOCAL:    goto LABEL_BC_PUSH_LOCAL;
    case BC_PUSH_ARGUMENT: goto LABEL_BC_PUSH_ARGUMENT;
    // ... one case per opcode
    default: __builtin_unreachable();
  }
#else
  goto* loopTargets[currentBytecodes[bytecodeIndexGlobal]];
#endif
```

Also add `-fno-jump-tables` to compile options (§1) so the compiler does not lower
the `switch` back into an indirect-jump table.

### 5  C++ global constructors and the shadow stack

Yk's shadow-stack pass instruments every function.  Functions called from C++ global
constructors run **before `main()`** initialises the shadow stack, which will
corrupt memory.

**Fix A — eliminate static-storage `std::string` members.**  Replace
`static const std::string` members of classes with static storage duration with
`static constexpr const char*` to eliminate the ctor.

**Fix B — lazy singleton for static instances.**  Convert any class with a static
global instance whose constructor calls instrumented code to a function-local static,
so construction is deferred until after `main()` runs:

```cpp
// Replace: static PrimitiveLoader loader;  (eager global ctor)
// With a lazy accessor:
static PrimitiveLoader& GetLoader() {
    static PrimitiveLoader instance;   // initialized on first call
    return instance;
}
// All static methods: loader.foo(x) → GetLoader().foo(x)
```

### 6  Private-linkage globals (ykrt change)

`--yk-embed-ir` serialises every LLVM global into the binary.  At runtime,
`GlobalDecl::eval()` in ykrt resolves each global's address via `dlsym()`.
Private/local-linkage globals — string literals (`.str.N`), `__PRETTY_FUNCTION__`
constants — are not in the dynamic symbol table, so `dlsym` returns null.

The linker pass also creates `__yk_globalvar_ptrs`: an array indexed by global
declaration order containing the runtime address of every global including private
ones.  Update `eval_globaldecls` to use it as a fallback:

```rust
// ykrt/src/compile/jitc_yk/aot_ir.rs

pub(crate) fn eval_globaldecls(&mut self) {
    let cn = CString::new("__yk_globalvar_ptrs").unwrap();
    let gvar_ptrs = unsafe {
        libc::dlsym(std::ptr::null_mut(), cn.as_c_str().as_ptr())
    } as *const *const c_void;

    for (idx, g) in self.global_decls.iter_mut().enumerate() {
        g.eval(idx, gvar_ptrs);
    }
}

pub(crate) fn eval(&mut self, idx: usize, gvar_ptrs: *const *const c_void) {
    let cn = CString::new(&*self.name).unwrap();
    let ptr = unsafe {
        libc::dlsym(std::ptr::null_mut(), cn.as_c_str().as_ptr())
    } as *const c_void;
    if !ptr.is_null() {
        self.eval = Some(ConstVal {
            tyidx: self.tyidx,
            bytes: Vec::from((ptr as usize).to_ne_bytes()),
        });
        return;
    }
    // Fall back to __yk_globalvar_ptrs[idx] for private/local-linkage globals.
    if !gvar_ptrs.is_null() {
        let fallback = unsafe { *gvar_ptrs.add(idx) };
        if !fallback.is_null() {
            self.eval = Some(ConstVal {
                tyidx: self.tyidx,
                bytes: Vec::from((fallback as usize).to_ne_bytes()),
            });
        }
    }
    // TLS globals remain unevaluated — the JIT handles them separately.
}
```

### 7  1-bit stores in the x64 JIT backend (ykrt change)

LTO keeps `bool` temporaries as `i1` alloca slots with explicit stores.
The x64 backend has no 1-bit move instruction; widen `i1` to `i8` (a `bool`'s
backing allocation is always at least 1 byte):

```rust
// ykrt/src/compile/j2/x64/x64hir_to_asm.rs
// register-value store branch:
match val_bitw {
    1 | 8 => IcedInst::with2(Code::Mov_rm8_r8, memop, valr.to_reg8()),
    16    => IcedInst::with2(Code::Mov_rm16_r16, memop, valr.to_reg16()),
    32    => IcedInst::with2(Code::Mov_rm32_r32, memop, valr.to_reg32()),
    64    => IcedInst::with2(Code::Mov_rm64_r64, memop, valr.to_reg64()),
    x     => todo!("{x}"),
}
// immediate-value store branch:
match val_bitw {
    1 | 8 => IcedInst::with2(Code::Mov_rm8_imm8, memop, imm),
    16    => IcedInst::with2(Code::Mov_rm16_imm16, memop, imm),
    32    => IcedInst::with2(Code::Mov_rm32_imm32, memop, imm),
    64    => IcedInst::with2(Code::Mov_rm64_imm32, memop, imm),
    x     => todo!("{x}"),
}
```

---

## Runtime verification

```sh
# Non-Yk build should pass all tests unaffected:
just test-som

# Yk build should print "Hello, World from SOM":
just hello-yk

# Verify tracing actually fires (all-zero stats means tracing never ran):
YK_HOT_THRESHOLD=5 YKD_LOG_STATS=ykstats.json \
  cmake-yk/SOM++ -cp Smalltalk Examples/VeryHeavyLoop.som
cat ykstats.json
# Expected: traces_recorded_ok > 0, traces_compiled_ok > 0, trace_executions > 0.
```
