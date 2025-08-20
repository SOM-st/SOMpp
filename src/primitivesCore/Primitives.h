#pragma once

#include "../vmobjects/ObjectFormats.h"

using FramePrimitiveRoutine = void(VMFrame*);
using UnaryPrimitiveRoutine = vm_oop_t(vm_oop_t);
using BinaryPrimitiveRoutine = vm_oop_t(vm_oop_t, vm_oop_t);
using TernaryPrimitiveRoutine = vm_oop_t(vm_oop_t, vm_oop_t, vm_oop_t);

class Prim {
public:
    explicit Prim(bool classSide, size_t bytecodeHash)
        : isClassSide(classSide), bytecodeHash(bytecodeHash) {}

    bool isClassSide;
    size_t bytecodeHash;
};

class FramePrim : public Prim {
public:
    FramePrim(FramePrimitiveRoutine ptr, bool classSide, size_t hash)
        : Prim(classSide, hash), pointer(ptr) {}
    explicit FramePrim() : Prim(false, 0), pointer(nullptr) {}

    [[nodiscard]] bool IsValid() const { return pointer != nullptr; }
    void MarkObjectAsInvalid() { pointer = nullptr; }

    void (*pointer)(VMFrame*);
};

class UnaryPrim : public Prim {
public:
    UnaryPrim(UnaryPrimitiveRoutine ptr, bool classSide, size_t hash)
        : Prim(classSide, hash), pointer(ptr) {}
    explicit UnaryPrim() : Prim(false, 0), pointer(nullptr) {}

    [[nodiscard]] bool IsValid() const { return pointer != nullptr; }
    void MarkObjectAsInvalid() { pointer = nullptr; }

    vm_oop_t (*pointer)(vm_oop_t);
};

class BinaryPrim : public Prim {
public:
    BinaryPrim(BinaryPrimitiveRoutine ptr, bool classSide, size_t hash)
        : Prim(classSide, hash), pointer(ptr) {}
    explicit BinaryPrim() : Prim(false, 0), pointer(nullptr) {}

    [[nodiscard]] bool IsValid() const { return pointer != nullptr; }

    void MarkObjectAsInvalid() { pointer = nullptr; }

    vm_oop_t (*pointer)(vm_oop_t, vm_oop_t);
};

class TernaryPrim : public Prim {
public:
    TernaryPrim(TernaryPrimitiveRoutine ptr, bool classSide, size_t hash)
        : Prim(classSide, hash), pointer(ptr) {}
    explicit TernaryPrim() : Prim(false, 0), pointer(nullptr) {}

    [[nodiscard]] bool IsValid() const { return pointer != nullptr; }

    void MarkObjectAsInvalid() { pointer = nullptr; }

    vm_oop_t (*pointer)(vm_oop_t, vm_oop_t, vm_oop_t);
};
