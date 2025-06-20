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

#include "PrimitiveContainer.h"

#include <cassert>
#include <cstddef>
#include <iostream>
#include <map>
#include <string>
#include <utility>

#include "../vm/Symbols.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMInvokable.h"
#include "../vmobjects/VMPrimitive.h"
#include "../vmobjects/VMSafePrimitive.h"
#include "../vmobjects/VMSymbol.h"
#include "Primitives.h"

void PrimitiveContainer::Add(const char* name,
                             FramePrimitiveRoutine routine,
                             bool classSide) {
    assert(framePrims.find(name) == framePrims.end());
    framePrims[std::string(name)] = {FramePrim(routine, classSide, 0),
                                     FramePrim()};
}
void PrimitiveContainer::Add(const char* name,
                             BinaryPrimitiveRoutine routine,
                             bool classSide) {
    assert(binaryPrims.find(name) == binaryPrims.end());
    binaryPrims[std::string(name)] = {BinaryPrim(routine, classSide, 0),
                                      BinaryPrim()};
}

void PrimitiveContainer::Add(const char* name,
                             UnaryPrimitiveRoutine routine,
                             bool classSide) {
    assert(unaryPrims.find(name) == unaryPrims.end());
    unaryPrims[std::string(name)] = {UnaryPrim(routine, classSide, 0),
                                     UnaryPrim()};
}

void PrimitiveContainer::Add(const char* name,
                             TernaryPrimitiveRoutine routine,
                             bool classSide) {
    assert(ternaryPrims.find(name) == ternaryPrims.end());
    ternaryPrims[std::string(name)] = {TernaryPrim(routine, classSide, 0),
                                       TernaryPrim()};
}

void PrimitiveContainer::Add(const char* name, FramePrimitiveRoutine routine,
                             bool classSide, size_t hash) {
    assert(framePrims.find(name) == framePrims.end());
    framePrims[std::string(name)] = {FramePrim(routine, classSide, hash),
                                     FramePrim()};
}

void PrimitiveContainer::Add(const char* name, BinaryPrimitiveRoutine routine,
                             bool classSide, size_t hash) {
    assert(binaryPrims.find(name) == binaryPrims.end());
    binaryPrims[std::string(name)] = {BinaryPrim(routine, classSide, hash),
                                      BinaryPrim()};
}

void PrimitiveContainer::Add(const char* name, UnaryPrimitiveRoutine routine,
                             bool classSide, size_t hash) {
    assert(unaryPrims.find(name) == unaryPrims.end());
    unaryPrims[std::string(name)] = {UnaryPrim(routine, classSide, hash),
                                     UnaryPrim()};
}

void PrimitiveContainer::Add(const char* name, TernaryPrimitiveRoutine routine,
                             bool classSide, size_t hash) {
    assert(ternaryPrims.find(name) == ternaryPrims.end());
    ternaryPrims[std::string(name)] = {TernaryPrim(routine, classSide, hash),
                                       TernaryPrim()};
}

void PrimitiveContainer::Add(const char* name, bool classSide,
                             FramePrimitiveRoutine routine1, size_t hash1,
                             FramePrimitiveRoutine routine2, size_t hash2) {
    assert(framePrims.find(name) == framePrims.end());
    framePrims[std::string(name)] = {FramePrim(routine1, classSide, hash1),
                                     FramePrim(routine2, classSide, hash2)};
}

void PrimitiveContainer::Add(const char* name, bool classSide,
                             BinaryPrimitiveRoutine routine1, size_t hash1,
                             BinaryPrimitiveRoutine routine2, size_t hash2) {
    assert(binaryPrims.find(name) == binaryPrims.end());
    binaryPrims[std::string(name)] = {BinaryPrim(routine1, classSide, hash1),
                                      BinaryPrim(routine2, classSide, hash2)};
}

void PrimitiveContainer::Add(const char* name, bool classSide,
                             UnaryPrimitiveRoutine routine1, size_t hash1,
                             UnaryPrimitiveRoutine routine2, size_t hash2) {
    assert(unaryPrims.find(name) == unaryPrims.end());
    unaryPrims[std::string(name)] = {UnaryPrim(routine1, classSide, hash1),
                                     UnaryPrim(routine2, classSide, hash2)};
}

void PrimitiveContainer::Add(const char* name, bool classSide,
                             TernaryPrimitiveRoutine routine1, size_t hash1,
                             TernaryPrimitiveRoutine routine2, size_t hash2) {
    assert(ternaryPrims.find(name) == ternaryPrims.end());
    ternaryPrims[std::string(name)] = {TernaryPrim(routine1, classSide, hash1),
                                       TernaryPrim(routine2, classSide, hash2)};
}

template <class PrimT>
void PrimitiveContainer::installPrimitives(
    bool classSide, bool showWarning, VMClass* clazz,
    std::map<std::string, std::pair<PrimT, PrimT>>& prims,
    VMInvokable* (*makePrimFn)(VMSymbol* sig, PrimT)) {
    for (auto const& p : prims) {
        PrimT prim1 = std::get<0>(p.second);
        PrimT prim2 = std::get<0>(p.second);

        assert(prim1.IsValid());
        if (classSide != prim1.isClassSide) {
            continue;
        }

        VMSymbol* sig = SymbolFor(p.first);

        PrimInstallResult const result = clazz->InstallPrimitive(
            makePrimFn(sig, prim1), prim1.bytecodeHash, !prim2.IsValid());
        if (result == PrimInstallResult::INSTALLED_ADDED && showWarning) {
            cout << "Warn: Primitive " << p.first
                 << " is not in class definition for class "
                 << clazz->GetName()->GetStdString() << '\n';
        } else if (result == PrimInstallResult::HASH_MISMATCH &&
                   prim2.IsValid()) {
            assert(prim1.isClassSide == prim2.isClassSide);
            clazz->InstallPrimitive(makePrimFn(sig, prim2), prim2.bytecodeHash,
                                    true);
        }
    }
}

void PrimitiveContainer::InstallPrimitives(VMClass* clazz, bool classSide,
                                           bool showWarning) {
    installPrimitives(classSide, showWarning, clazz, unaryPrims,
                      VMSafePrimitive::GetSafeUnary);
    installPrimitives(classSide, showWarning, clazz, binaryPrims,
                      VMSafePrimitive::GetSafeBinary);
    installPrimitives(classSide, showWarning, clazz, ternaryPrims,
                      VMSafePrimitive::GetSafeTernary);
    installPrimitives(classSide, showWarning, clazz, framePrims,
                      VMPrimitive::GetFramePrim);
}
