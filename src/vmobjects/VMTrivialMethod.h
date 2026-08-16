#pragma once

#include "../compiler/MethodGenerationContext.h"
#include "../vm/Globals.h"
#include "ObjectFormats.h"
#include "Signature.h"
#include "VMInvokable.h"
#include "VMSymbol.h"

class VMTrivialMethod : public VMInvokable {
public:
    typedef GCTrivialMethod Stored;

    VMTrivialMethod(VMSymbol* sig, vector<Variable>& arguments)
        : VMInvokable(sig), arguments(arguments) {}

    [[nodiscard]] VMClass* GetClass() const final {
        return load_ptr(methodClass);
    }

    [[nodiscard]] bool IsPrimitive() const final { return false; };

    [[nodiscard]] bool RequiresClosureContext() const override { return true; }

    void MergeScopeInto(MethodGenerationContext& mgenc) final {
        if (!arguments.empty()) {
            mgenc.InlineAsLocals(arguments);
        }
    }

    const Variable* GetArgument(size_t index, size_t contextLevel) final {
        assert(contextLevel == 0);
        if (contextLevel > 0) {
            return nullptr;
        }
        return &arguments.at(index);
    }

    [[nodiscard]] inline uint8_t GetNumberOfArguments() const final {
        return Signature::GetNumberOfArguments(load_ptr(signature));
    }

    void Dump(const char* indent, bool printObjects) override;

private:
    vector<Variable> arguments;
};

VMTrivialMethod* MakeLiteralReturn(VMSymbol* sig, vector<Variable>& arguments,
                                   vm_oop_t literal);
VMTrivialMethod* MakeGlobalReturn(VMSymbol* sig, vector<Variable>& arguments,
                                  VMSymbol* globalName);
VMTrivialMethod* MakeGetter(VMSymbol* sig, vector<Variable>& arguments,
                            size_t fieldIndex);
VMTrivialMethod* MakeSetter(VMSymbol* sig, vector<Variable>& arguments,
                            size_t fieldIndex, size_t argIndex);

class VMLiteralReturn : public VMTrivialMethod {
public:
    typedef GCLiteralReturn Stored;

    VMLiteralReturn(VMSymbol* sig, vector<Variable>& arguments,
                    vm_oop_t literal)
        : VMTrivialMethod(sig, arguments),
          literal(store_with_separate_barrier(literal)),
          numberOfArguments(Signature::GetNumberOfArguments(sig)) {
        write_barrier(this, sig);
        write_barrier(this, literal);
    }

    // just pushes a literal, never touches self
    [[nodiscard]] bool RequiresClosureContext() const override { return false; }

    [[nodiscard]] inline size_t GetObjectSize() const override {
        return sizeof(VMLiteralReturn);
    }

    VMFrame* Invoke(VMFrame* /*frame*/) override;
    VMFrame* Invoke1(VMFrame* /*frame*/) override;

    void InlineInto(MethodGenerationContext& mgenc, const Parser& parser,
                    bool mergeScope = true) final;

    [[nodiscard]] AbstractVMObject* CloneForMovingGC() const final;

    void MarkObjectAsInvalid() final {
        VMTrivialMethod::MarkObjectAsInvalid();
        literal = INVALID_GC_POINTER;
    }

    void WalkObjects(walk_heap_fn /*walk*/) override;

    [[nodiscard]] bool IsMarkedInvalid() const final {
        return literal == INVALID_GC_POINTER;
    }

    [[nodiscard]] std::string AsDebugString() const final;

private:
    gc_oop_t literal;
    uint8_t numberOfArguments;
};

class VMGlobalReturn : public VMTrivialMethod {
public:
    typedef GCGlobalReturn Stored;

    VMGlobalReturn(VMSymbol* sig, vector<Variable>& arguments,
                   VMSymbol* globalName)
        : VMTrivialMethod(sig, arguments),
          globalName(store_with_separate_barrier(globalName)),
          numberOfArguments(Signature::GetNumberOfArguments(sig)) {
        write_barrier(this, sig);
        write_barrier(this, globalName);
    }

    [[nodiscard]] inline size_t GetObjectSize() const override {
        return sizeof(VMGlobalReturn);
    }

    VMFrame* Invoke(VMFrame* /*frame*/) override;
    VMFrame* Invoke1(VMFrame* /*frame*/) override;

    void InlineInto(MethodGenerationContext& mgenc, const Parser& parser,
                    bool mergeScope = true) final;

    [[nodiscard]] AbstractVMObject* CloneForMovingGC() const final;

    void MarkObjectAsInvalid() final {
        VMTrivialMethod::MarkObjectAsInvalid();
        globalName = (GCSymbol*)INVALID_GC_POINTER;
    }

    void WalkObjects(walk_heap_fn /*walk*/) override;

    [[nodiscard]] bool IsMarkedInvalid() const final {
        return globalName == (GCSymbol*)INVALID_GC_POINTER;
    }

    [[nodiscard]] std::string AsDebugString() const final;

private:
    GCSymbol* globalName;
    uint8_t numberOfArguments;
};

class VMGetter : public VMTrivialMethod {
public:
    typedef GCGetter Stored;

    VMGetter(VMSymbol* sig, vector<Variable>& arguments, size_t fieldIndex)
        : VMTrivialMethod(sig, arguments), fieldIndex(fieldIndex),
          numberOfArguments(Signature::GetNumberOfArguments(sig)) {
        write_barrier(this, sig);
    }

    [[nodiscard]] inline size_t GetObjectSize() const override {
        return sizeof(VMGetter);
    }

    VMFrame* Invoke(VMFrame* /*frame*/) override;
    VMFrame* Invoke1(VMFrame* /*frame*/) override;

    void InlineInto(MethodGenerationContext& mgenc, const Parser& parser,
                    bool mergeScope = true) final;

    [[nodiscard]] AbstractVMObject* CloneForMovingGC() const final;

    void MarkObjectAsInvalid() final { VMTrivialMethod::MarkObjectAsInvalid(); }

    [[nodiscard]] bool IsMarkedInvalid() const final {
        return signature == (GCSymbol*)INVALID_GC_POINTER;
    }

    void WalkObjects(walk_heap_fn /*walk*/) override;

    [[nodiscard]] std::string AsDebugString() const final;

private:
    size_t fieldIndex;
    uint8_t numberOfArguments;
};

class VMSetter : public VMTrivialMethod {
public:
    typedef GCSetter Stored;

    VMSetter(VMSymbol* sig, vector<Variable>& arguments, size_t fieldIndex,
             size_t argIndex)
        : VMTrivialMethod(sig, arguments), fieldIndex(fieldIndex),
          argIndex(argIndex),
          numberOfArguments(Signature::GetNumberOfArguments(sig)) {
        write_barrier(this, sig);
    }

    [[nodiscard]] inline size_t GetObjectSize() const override {
        return sizeof(VMSetter);
    }

    VMFrame* Invoke(VMFrame* /*frame*/) override;
    VMFrame* Invoke1(VMFrame* /*unused*/) override;

    void InlineInto(MethodGenerationContext& mgenc, const Parser& parser,
                    bool mergeScope = true) final;

    [[nodiscard]] AbstractVMObject* CloneForMovingGC() const final;

    void MarkObjectAsInvalid() final { VMTrivialMethod::MarkObjectAsInvalid(); }

    [[nodiscard]] bool IsMarkedInvalid() const final {
        return signature == (GCSymbol*)INVALID_GC_POINTER;
    }

    void WalkObjects(walk_heap_fn /*walk*/) override;

    [[nodiscard]] std::string AsDebugString() const final;

private:
    make_testable(public);

    size_t fieldIndex;
    size_t argIndex;
    uint8_t numberOfArguments;
};
