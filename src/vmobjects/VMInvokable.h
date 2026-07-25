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

#include "ObjectFormats.h"
#include "VMObject.h"
#include "VMSymbol.h"

class MethodGenerationContext;
class Variable;
class VMFrame;
class Parser;

class VMInvokable : public AbstractVMObject {
public:
    typedef GCInvokable Stored;

    explicit VMInvokable(VMSymbol* sig)
        : hash((intptr_t)this), signature(store_with_separate_barrier(sig)) {}

    [[nodiscard]] int64_t GetHash() const override { return hash; }

    virtual VMFrame* Invoke(VMFrame*) = 0;
    virtual VMFrame* Invoke1(VMFrame*) = 0;
    virtual void InlineInto(MethodGenerationContext& mgenc,
                            const Parser& parser, bool mergeScope = true) = 0;
    virtual void MergeScopeInto(
        MethodGenerationContext& /*unused*/) { /* NOOP for everything but
                                                  VMMethods */
    }
    virtual void AdaptAfterOuterInlined(
        uint8_t removedCtxLevel,
        MethodGenerationContext&
            mgencWithInlined) { /* NOOP for everything but VMMethods */ }
    virtual const Variable* GetArgument(size_t /*unused*/, size_t /*unused*/);

#ifdef FRAME_OPTIMIZATION
    [[nodiscard]] virtual bool RequiresClosureContext() const { return false; }
#endif

    [[nodiscard]] virtual uint8_t GetNumberOfArguments() const = 0;

    [[nodiscard]] virtual bool IsPrimitive() const;

    [[nodiscard]] inline VMSymbol* GetSignature() const {
        return load_ptr(signature);
    }

    [[nodiscard]] VMClass* GetHolder() const { return load_ptr(holder); }

    virtual void SetHolder(VMClass* hld);

    void WalkObjects(walk_heap_fn /*unused*/) override;

    void MarkObjectAsInvalid() override {
        signature = (GCSymbol*)INVALID_GC_POINTER;
        holder = (GCClass*)INVALID_GC_POINTER;
    }

    virtual void Dump(const char* indent, bool printObjects) = 0;

protected:
    make_testable(public);

    int64_t hash;

    GCSymbol* signature;
    GCClass* holder{nullptr};
};
