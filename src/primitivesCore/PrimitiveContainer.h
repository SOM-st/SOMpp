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

#include <iostream>
#include <map>

#include "../misc/defs.h"
#include "Primitives.h"

/// Base class for all container objects holding SOM++ primitives.
// Primitive container classes need to initialize a std::map<std::string,
// PrimitiveRoutine*> in order to map smalltalk message names to the method
// to call.
class PrimitiveContainer {
public:
    PrimitiveContainer() = default;
    virtual ~PrimitiveContainer() = default;

    void InstallPrimitives(VMClass* clazz, bool classSide, bool showWarning);

    void Add(const char* name, FramePrimitiveRoutine /*routine*/,
             bool classSide);
    void Add(const char* name, UnaryPrimitiveRoutine /*routine*/,
             bool classSide);
    void Add(const char* name, BinaryPrimitiveRoutine /*routine*/,
             bool classSide);
    void Add(const char* name, TernaryPrimitiveRoutine /*routine*/,
             bool classSide);

    void Add(const char* name, FramePrimitiveRoutine /*routine*/,
             bool classSide, size_t hash);
    void Add(const char* name, UnaryPrimitiveRoutine /*routine*/,
             bool classSide, size_t hash);
    void Add(const char* name, BinaryPrimitiveRoutine /*routine*/,
             bool classSide, size_t hash);
    void Add(const char* name, TernaryPrimitiveRoutine /*routine*/,
             bool classSide, size_t hash);

    void Add(const char* name, bool classSide,
             FramePrimitiveRoutine /*routine*/, size_t hash1,
             FramePrimitiveRoutine /*routine*/, size_t hash2);
    void Add(const char* name, bool classSide,
             UnaryPrimitiveRoutine /*routine*/, size_t hash1,
             UnaryPrimitiveRoutine /*routine*/, size_t hash2);
    void Add(const char* name, bool classSide,
             BinaryPrimitiveRoutine /*routine*/, size_t hash1,
             BinaryPrimitiveRoutine /*routine*/, size_t hash2);
    void Add(const char* name, bool classSide,
             TernaryPrimitiveRoutine /*routine*/, size_t hash1,
             TernaryPrimitiveRoutine /*routine*/, size_t hash2);

private:
    std::map<std::string, std::pair<FramePrim, FramePrim>> framePrims;
    std::map<std::string, std::pair<UnaryPrim, UnaryPrim>> unaryPrims;
    std::map<std::string, std::pair<BinaryPrim, BinaryPrim>> binaryPrims;
    std::map<std::string, std::pair<TernaryPrim, TernaryPrim>> ternaryPrims;

    template <class PrimT>
    bool installPrimitives(
        bool classSide, bool showWarning, VMClass* clazz,
        std::map<std::string, std::pair<PrimT, PrimT>>& prims,
        VMInvokable* (*makePrimFn)(VMSymbol* sig, PrimT));
};
