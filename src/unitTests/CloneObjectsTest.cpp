/*
 * CloneObjectsTest.cpp
 *
 *  Created on: 21.01.2011
 *      Author: christian
 */

#include "CloneObjectsTest.h"

#include <cppunit/TestAssert.h>
#include <cstdint>
#include <vector>

#include "../compiler/LexicalScope.h"
#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../vm/Globals.h"
#include "../vm/Symbols.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMArray.h"
#include "../vmobjects/VMBlock.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMDouble.h"
#include "../vmobjects/VMEvaluationPrimitive.h"
#include "../vmobjects/VMInteger.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMObject.h"
#include "../vmobjects/VMPrimitive.h"
#include "../vmobjects/VMString.h"
#include "../vmobjects/VMSymbol.h"

void CloneObjectsTest::testCloneObject() {
    auto* orig = new (GetHeap<HEAP_CLS>(), 0) VMObject(0, sizeof(VMObject));
    VMObject* clone = orig->CloneForMovingGC();
    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(),
                                 clone->GetClass());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->GetObjectSize(),
                                 clone->GetObjectSize());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!",
                                 orig->GetNumberOfFields(),
                                 clone->GetNumberOfFields());

    CPPUNIT_ASSERT_EQUAL_MESSAGE("hash differs!!", orig->GetHash(),
                                 clone->GetHash());
}

void CloneObjectsTest::testCloneInteger() {
    VMInteger* orig = Universe::NewInteger(42);
    VMInteger* clone = orig->CloneForMovingGC();

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(),
                                 clone->GetClass());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("integer value differs!!",
                                 orig->embeddedInteger, clone->embeddedInteger);

    CPPUNIT_ASSERT_EQUAL_MESSAGE("hash differs!!", orig->GetHash(),
                                 clone->GetHash());
}

void CloneObjectsTest::testCloneDouble() {
    VMDouble* orig = Universe::NewDouble(123.4);
    VMDouble* clone = orig->CloneForMovingGC();

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(),
                                 clone->GetClass());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("double value differs!!", orig->embeddedDouble,
                                 clone->embeddedDouble);

    CPPUNIT_ASSERT_EQUAL_MESSAGE("hash differs!!", orig->GetHash(),
                                 clone->GetHash());
}

void CloneObjectsTest::testCloneString() {
    VMString* orig = Universe::NewString("foobar");
    VMString* clone = orig->CloneForMovingGC();

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(),
                                 clone->GetClass());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->GetObjectSize(),
                                 clone->GetObjectSize());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("string differs!!!", orig->GetStdString(),
                                 clone->GetStdString());

    CPPUNIT_ASSERT_EQUAL_MESSAGE("hash differs!!", orig->GetHash(),
                                 clone->GetHash());
    CPPUNIT_ASSERT_MESSAGE("internal string was not copied",
                           (intptr_t)orig->chars != (intptr_t)clone->chars);

    // change the string
    orig->chars[0] = 'm';
    CPPUNIT_ASSERT_MESSAGE("std::string should differ after changing string",
                           orig->GetStdString() != clone->GetStdString());

    CPPUNIT_ASSERT_MESSAGE("hash should change when changing string",
                           orig->GetHash() != clone->GetHash());
}

void CloneObjectsTest::testCloneSymbol() {
    VMSymbol* orig = NewSymbol("foobar");
    VMSymbol* clone = orig->CloneForMovingGC();

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->GetClass(),
                                 clone->GetClass());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->GetObjectSize(),
                                 clone->GetObjectSize());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("string differs!!!", orig->GetStdString(),
                                 clone->GetStdString());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("hash differs!!", orig->GetHash(),
                                 clone->GetHash());
}

void CloneObjectsTest::testCloneArray() {
    VMArray* orig = Universe::NewArray(3);
    orig->SetIndexableField(0, Universe::NewString("foobar42"));
    orig->SetIndexableField(1, Universe::NewString("foobar43"));
    orig->SetIndexableField(2, Universe::NewString("foobar44"));
    VMArray* clone = orig->CloneForMovingGC();

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->clazz, clone->clazz);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->totalObjectSize,
                                 clone->totalObjectSize);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!",
                                 orig->numberOfFields, clone->numberOfFields);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!",
                                 orig->GetNumberOfIndexableFields(),
                                 clone->GetNumberOfIndexableFields());

    CPPUNIT_ASSERT_EQUAL_MESSAGE("field 0 differs", orig->GetIndexableField(0),
                                 clone->GetIndexableField(0));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("field 1 differs", orig->GetIndexableField(1),
                                 clone->GetIndexableField(1));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("field 2 differs", orig->GetIndexableField(2),
                                 clone->GetIndexableField(2));

    CPPUNIT_ASSERT_EQUAL_MESSAGE("hash differs!!", orig->GetHash(),
                                 clone->GetHash());
}

void CloneObjectsTest::testCloneBlock() {
    VMSymbol* methodSymbol = NewSymbol("someMethod");

    vector<BackJump> inlinedLoops;
    VMMethod* method =
        Universe::NewMethod(methodSymbol, 0, 0, 0, 0,
                            new LexicalScope(nullptr, {}, {}), inlinedLoops);
    VMBlock* orig = Universe::NewBlock(method, Interpreter::GetFrame(),
                                       method->GetNumberOfArguments());
    VMBlock* clone = orig->CloneForMovingGC();

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->clazz, clone->clazz);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->totalObjectSize,
                                 clone->totalObjectSize);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!",
                                 orig->numberOfFields, clone->numberOfFields);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("blockMethod differs!!", orig->blockMethod,
                                 clone->blockMethod);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("context differs!!", orig->context,
                                 clone->context);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("hash differs!!", orig->GetHash(),
                                 clone->GetHash());
}
void CloneObjectsTest::testClonePrimitive() {
    VMSymbol* primitiveSymbol = NewSymbol("myPrimitive");
    auto* orig = dynamic_cast<VMPrimitive*>(
        VMPrimitive::GetEmptyPrimitive(primitiveSymbol, false));
    VMPrimitive* clone = orig->CloneForMovingGC();
    CPPUNIT_ASSERT_EQUAL_MESSAGE("signature differs!!", orig->signature,
                                 clone->signature);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("holder differs!!", orig->holder,
                                 clone->holder);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("routine differs!!", orig->prim.pointer,
                                 clone->prim.pointer);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("routine differs!!", orig->prim.isClassSide,
                                 clone->prim.isClassSide);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("hash differs!!", orig->GetHash(),
                                 clone->GetHash());
}

void CloneObjectsTest::testCloneEvaluationPrimitive() {
    auto* orig = new (GetHeap<HEAP_CLS>(), 0) VMEvaluationPrimitive(1);
    VMEvaluationPrimitive* clone = orig->CloneForMovingGC();

    CPPUNIT_ASSERT_EQUAL_MESSAGE("signature differs!!", orig->signature,
                                 clone->signature);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("holder differs!!", orig->holder,
                                 clone->holder);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfArguments differs!!",
                                 orig->numberOfArguments,
                                 clone->numberOfArguments);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("hash differs!!", orig->GetHash(),
                                 clone->GetHash());
}

void CloneObjectsTest::testCloneFrame() {
    VMSymbol* methodSymbol = NewSymbol("frameMethod");

    vector<BackJump> inlinedLoops;
    VMMethod* method =
        Universe::NewMethod(methodSymbol, 0, 0, 0, 0,
                            new LexicalScope(nullptr, {}, {}), inlinedLoops);

    VMFrame* orig = Universe::NewFrame(nullptr, method);
    VMFrame* context = orig->CloneForMovingGC();
    orig->SetContext(context);
    VMInteger* dummyArg = Universe::NewInteger(1111);
    orig->SetArgument(0, 0, dummyArg);
    VMFrame* clone = orig->CloneForMovingGC();

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->totalObjectSize,
                                 clone->totalObjectSize);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("GetPreviousFrame differs!!",
                                 orig->GetPreviousFrame(),
                                 clone->GetPreviousFrame());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("GetContext differs!!", orig->GetContext(),
                                 clone->GetContext());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOGetMethodfFields differs!!",
                                 orig->GetMethod(), clone->GetMethod());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("bytecodeIndex differs!!", orig->bytecodeIndex,
                                 clone->bytecodeIndex);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("hash differs!!", orig->GetHash(),
                                 clone->GetHash());
}

void CloneObjectsTest::testCloneMethod() {
    VMSymbol* methodSymbol = NewSymbol("myMethod");

    vector<BackJump> inlinedLoops;
    VMMethod* orig =
        Universe::NewMethod(methodSymbol, 0, 0, 0, 0,
                            new LexicalScope(nullptr, {}, {}), inlinedLoops);
    VMMethod* clone = orig->CloneForMovingGC();

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfLocals differs!!",
                                 orig->numberOfLocals, clone->numberOfLocals);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("bcLength differs!!", orig->bcLength,
                                 clone->bcLength);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("maximumNumberOfStackElements differs!!",
                                 orig->maximumNumberOfStackElements,
                                 clone->maximumNumberOfStackElements);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfArguments differs!!",
                                 orig->numberOfArguments,
                                 clone->numberOfArguments);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfConstants differs!!",
                                 orig->numberOfConstants,
                                 clone->numberOfConstants);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("hash differs!!", orig->GetHash(),
                                 clone->GetHash());

    CPPUNIT_ASSERT_EQUAL_MESSAGE("GetHolder() differs!!", orig->GetHolder(),
                                 clone->GetHolder());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("GetSignature() differs!!",
                                 orig->GetSignature(), clone->GetSignature());
}

void CloneObjectsTest::testCloneClass() {
    VMClass* orig = Universe::NewClass(load_ptr(integerClass));
    orig->SetName(NewSymbol("MyClass"));
    orig->SetSuperClass(load_ptr(doubleClass));
    orig->SetInstanceFields(Universe::NewArray(2));
    orig->SetInstanceInvokables(Universe::NewArray(4));
    VMClass* clone = orig->CloneForMovingGC();

    CPPUNIT_ASSERT((intptr_t)orig != (intptr_t)clone);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("class differs!!", orig->clazz, clone->clazz);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("objectSize differs!!", orig->totalObjectSize,
                                 clone->totalObjectSize);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("numberOfFields differs!!",
                                 orig->numberOfFields, clone->numberOfFields);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("superClass differs!!", orig->superClass,
                                 clone->superClass);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("name differs!!", orig->name, clone->name);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("instanceFields differs!!",
                                 orig->instanceFields, clone->instanceFields);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("instanceInvokables differs!!",
                                 orig->instanceInvokables,
                                 clone->instanceInvokables);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("hash differs!!", orig->GetHash(),
                                 clone->GetHash());
}
