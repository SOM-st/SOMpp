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

#include <vector>

#include "../misc/defs.h"
#include "../vm/Globals.h"
#include "../vm/IsValidObject.h"
#include "VMObject.h"

#if defined(_MSC_VER)  // Visual Studio
  #include <windows.h>

  #include "../primitives/Core.h"
#endif

class ClassGenerationContext;

enum PrimInstallResult : uint8_t {
    NULL_ARG,
    NULL_IN_INVOKABLES,
    HASH_MISMATCH,
    INSTALLED_REPLACED,
    INSTALLED_ADDED
};

class VMClass : public VMObject {
public:
    typedef GCClass Stored;

    VMClass();
    VMClass(size_t numberOfFields, size_t additionalBytes);

    [[nodiscard]] VMObject* GetSuperClass() const;
    void SetSuperClass(VMObject* sup);
    [[nodiscard]] bool HasSuperClass() const;
    [[nodiscard]] VMSymbol* GetName() const;
    void SetName(VMSymbol* name);
    [[nodiscard]] VMArray* GetInstanceFields() const;
    void SetInstanceFields(VMArray* instFields);
    [[nodiscard]] VMArray* GetInstanceInvokables() const;
    void SetInstanceInvokables(VMArray* /*invokables*/);
    [[nodiscard]] size_t GetNumberOfInstanceInvokables() const;
    [[nodiscard]] VMInvokable* GetInstanceInvokable(size_t index) const;
    void SetInstanceInvokable(size_t index, VMInvokable* invokable);
    VMInvokable* LookupInvokable(VMSymbol* name);
    int64_t LookupFieldIndex(VMSymbol* name) const;
    PrimInstallResult InstallPrimitive(VMInvokable* invokable,
                                       size_t bytecodeHash,
                                       bool reportHashMismatch);
    [[nodiscard]] VMSymbol* GetInstanceFieldName(size_t index) const;
    [[nodiscard]] size_t GetNumberOfInstanceFields() const;
    [[nodiscard]] bool HasPrimitives() const;
    void LoadPrimitives(bool showWarning);
    [[nodiscard]] VMClass* CloneForMovingGC() const override;

    [[nodiscard]] std::string AsDebugString() const override;

private:
    static bool hasPrimitivesFor(const std::string& cl);
    void setPrimitives(const std::string& cname, bool classSide);
    [[nodiscard]] size_t numberOfSuperInstanceFields() const;

    static const size_t VMClassNumberOfFields;

    make_testable(public);

    // Remember to update Parser::superclass when the fields are changed
    // Theses are treated like VMObject fields and initialized that way
    GCSymbol* name;
    GCArray* instanceFields;
    GCArray* instanceInvokables;
    GCObject* superClass;
};
