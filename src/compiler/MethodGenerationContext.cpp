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

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>

#include "../misc/VectorUtil.h"
#include "../vm/Symbols.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMPrimitive.h"
#include "../vmobjects/VMSymbol.h"
#include "MethodGenerationContext.h"

MethodGenerationContext::MethodGenerationContext() :
        holderGenc(nullptr), outerGenc(nullptr),
        blockMethod(false), signature(nullptr), primitive(false), finished(false),
        currentStackDepth(0), maxStackDepth(0) { }

VMMethod* MethodGenerationContext::Assemble() {
    // create a method instance with the given number of bytecodes and literals
    size_t numLiterals = literals.size();
    size_t numLocals = locals.size();
    VMMethod* meth = GetUniverse()->NewMethod(signature, bytecode.size(),
            numLiterals, numLocals, maxStackDepth);

    // copy literals into the method
    for (size_t i = 0; i < numLiterals; i++) {
        vm_oop_t l = literals[i];
        meth->SetIndexableField(i, l);
    }

    // copy bytecodes into method
    size_t bc_size = bytecode.size();
    for (size_t i = 0; i < bc_size; i++) {
        meth->SetBytecode(i, bytecode[i]);
    }

    // return the method - the holder field is to be set later on!
    return meth;
}

VMPrimitive* MethodGenerationContext::AssemblePrimitive(bool classSide) {
    return VMPrimitive::GetEmptyPrimitive(signature, classSide);
}

MethodGenerationContext::~MethodGenerationContext() {
}

int8_t MethodGenerationContext::FindLiteralIndex(vm_oop_t lit) {
    return (int8_t)IndexOf(literals, lit);
}

uint8_t MethodGenerationContext::GetFieldIndex(VMSymbol* field) {
    int16_t idx = holderGenc->GetFieldIndex(field);
    assert(idx >= 0);
    return idx;
}

bool MethodGenerationContext::FindVar(VMSymbol* var, int64_t* index,
        int* context, bool* isArgument) {
    if ((*index = IndexOf(locals, var)) == -1) {
        if ((*index = IndexOf(arguments, var)) == -1) {
            if (!outerGenc)
                return false;
            else {
                (*context)++;
                return outerGenc->FindVar(var, index, context, isArgument);
            }
        } else
            *isArgument = true;
    }

    return true;
}

bool MethodGenerationContext::HasField(VMSymbol* field) {
    return holderGenc->HasField(field);
}

size_t MethodGenerationContext::GetNumberOfArguments() {
    return arguments.size();
}

void MethodGenerationContext::SetHolder(ClassGenerationContext* holder) {
    holderGenc = holder;
}

void MethodGenerationContext::SetOuter(MethodGenerationContext* outer) {
    outerGenc = outer;
}

void MethodGenerationContext::SetIsBlockMethod(bool isBlock) {
    blockMethod = isBlock;
}

void MethodGenerationContext::SetSignature(VMSymbol* sig) {
    signature = sig;
}

void MethodGenerationContext::SetPrimitive(bool prim) {
    primitive = prim;
}

void MethodGenerationContext::AddArgument(const std::string& arg) {
    VMSymbol* argSym = SymbolFor(arg);
    arguments.push_back(argSym);
}

void MethodGenerationContext::AddLocal(const std::string& local) {
    VMSymbol* localSym =SymbolFor(local);
    locals.push_back(localSym);
}

uint8_t MethodGenerationContext::AddLiteral(vm_oop_t lit) {
    assert(!AS_OBJ(lit)->IsMarkedInvalid());

    uint8_t idx = literals.size();
    literals.push_back(lit);
    return idx;
}

int8_t MethodGenerationContext::AddLiteralIfAbsent(vm_oop_t lit) {
    int8_t idx = IndexOf(literals, lit);
    if (idx != -1) {
        assert(idx >= 0 && (size_t)idx < literals.size() && "Expect index to be inside the literals vector.");
        return idx;
    }
    return AddLiteral(lit);
}

void MethodGenerationContext::UpdateLiteral(vm_oop_t oldValue, uint8_t index, vm_oop_t newValue) {
    assert(literals.at(index) == oldValue);
    literals[index] = newValue;
}

bool MethodGenerationContext::AddArgumentIfAbsent(const std::string& arg) {
    VMSymbol* argSym = SymbolFor(arg);
    if (Contains(locals, argSym)) {
        return false;
    }
    arguments.push_back(argSym);
    return true;
}

bool MethodGenerationContext::AddLocalIfAbsent(const std::string& local) {
    VMSymbol* localSym = SymbolFor(local);
    if (Contains(locals, localSym)) {
        return false;
    }
    locals.push_back(localSym);
    return true;
}

void MethodGenerationContext::SetFinished(bool finished) {
    this->finished = finished;
}

ClassGenerationContext* MethodGenerationContext::GetHolder() {
    return holderGenc;
}

MethodGenerationContext* MethodGenerationContext::GetOuter() {
    return outerGenc;
}

VMSymbol* MethodGenerationContext::GetSignature() {
    return signature;
}

bool MethodGenerationContext::IsPrimitive() {
    return primitive;
}

bool MethodGenerationContext::IsBlockMethod() {
    return blockMethod;
}

bool MethodGenerationContext::IsFinished() {
    return finished;
}

bool MethodGenerationContext::HasBytecodes() {
    return !bytecode.empty();
}

size_t MethodGenerationContext::AddBytecode(uint8_t bc, size_t stackEffect) {
    currentStackDepth += stackEffect;
    maxStackDepth = max(maxStackDepth, currentStackDepth);

    bytecode.push_back(bc);
    return bytecode.size();
}

size_t MethodGenerationContext::AddBytecodeArgument(uint8_t bc) {
    bytecode.push_back(bc);
    return bytecode.size();
}
