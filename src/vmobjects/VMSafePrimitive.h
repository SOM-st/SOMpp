#pragma once

#include "../primitivesCore/PrimitiveContainer.h"
#include "Signature.h"
#include "VMInvokable.h"

class VMSafePrimitive : public VMInvokable {
public:
    typedef GCSafePrimitive Stored;

    explicit VMSafePrimitive(VMSymbol* sig) : VMInvokable(sig) {}

    [[nodiscard]] VMClass* GetClass() const final {
        return load_ptr(primitiveClass);
    }

    [[nodiscard]] bool IsPrimitive() const final { return true; };

    void InlineInto(MethodGenerationContext& mgenc,
                    bool mergeScope = true) final;

    static VMInvokable* GetSafeUnary(VMSymbol* sig, UnaryPrim prim);
    static VMInvokable* GetSafeBinary(VMSymbol* sig, BinaryPrim prim);
    static VMInvokable* GetSafeTernary(VMSymbol* sig, TernaryPrim prim);

    [[nodiscard]] std::string AsDebugString() const final;

    [[nodiscard]] inline uint8_t GetNumberOfArguments() const final {
        return Signature::GetNumberOfArguments(load_ptr(signature));
    }

    void Dump(const char* indent, bool printObjects) override;
};

class VMSafeUnaryPrimitive : public VMSafePrimitive {
public:
    typedef GCSafeUnaryPrimitive Stored;

    VMSafeUnaryPrimitive(VMSymbol* sig, UnaryPrim prim)
        : VMSafePrimitive(sig), prim(prim) {
        write_barrier(this, sig);
    }

    [[nodiscard]] inline size_t GetObjectSize() const override {
        return sizeof(VMSafeUnaryPrimitive);
    }

    VMFrame* Invoke(VMFrame* /*frame*/) override;
    VMFrame* Invoke1(VMFrame* /*frame*/) override;

    [[nodiscard]] AbstractVMObject* CloneForMovingGC() const final;

    void MarkObjectAsInvalid() final {
        VMSafePrimitive::MarkObjectAsInvalid();
        prim.MarkObjectAsInvalid();
    }

    [[nodiscard]] bool IsMarkedInvalid() const final { return !prim.IsValid(); }

private:
    UnaryPrim prim;
};

class VMSafeBinaryPrimitive : public VMSafePrimitive {
public:
    typedef GCSafeBinaryPrimitive Stored;

    VMSafeBinaryPrimitive(VMSymbol* sig, BinaryPrim prim)
        : VMSafePrimitive(sig), prim(prim) {
        write_barrier(this, sig);
    }

    [[nodiscard]] inline size_t GetObjectSize() const override {
        return sizeof(VMSafeBinaryPrimitive);
    }

    VMFrame* Invoke(VMFrame* /*frame*/) override;
    VMFrame* Invoke1(VMFrame* /*unused*/) override;

    [[nodiscard]] AbstractVMObject* CloneForMovingGC() const final;

    void MarkObjectAsInvalid() final {
        VMSafePrimitive::MarkObjectAsInvalid();
        prim.MarkObjectAsInvalid();
    }

    [[nodiscard]] bool IsMarkedInvalid() const final { return !prim.IsValid(); }

private:
    BinaryPrim prim;
};

class VMSafeTernaryPrimitive : public VMSafePrimitive {
public:
    typedef GCSafeTernaryPrimitive Stored;

    VMSafeTernaryPrimitive(VMSymbol* sig, TernaryPrim prim)
        : VMSafePrimitive(sig), prim(prim) {
        write_barrier(this, sig);
    }

    [[nodiscard]] inline size_t GetObjectSize() const override {
        return sizeof(VMSafeTernaryPrimitive);
    }

    VMFrame* Invoke(VMFrame* /*frame*/) override;
    VMFrame* Invoke1(VMFrame* /*unused*/) override;

    [[nodiscard]] AbstractVMObject* CloneForMovingGC() const final;

    void MarkObjectAsInvalid() final {
        VMSafePrimitive::MarkObjectAsInvalid();
        prim.MarkObjectAsInvalid();
    }

    [[nodiscard]] bool IsMarkedInvalid() const final { return !prim.IsValid(); }

private:
    TernaryPrim prim;
};
