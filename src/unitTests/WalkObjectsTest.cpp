/*
 * WalkObjectsTest.cpp
 *
 *  Created on: 12.01.2011
 *      Author: christian
 */
#include "WalkObjectsTest.h"

#include <algorithm>
#include <cppunit/TestAssert.h>
#include <cstddef>
#include <vector>

#include "../compiler/LexicalScope.h"
#include "../compiler/Variable.h"
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
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMInteger.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMPrimitive.h"
#include "../vmobjects/VMSymbol.h"

static const size_t NoOfFields_Object = 1;
static const size_t NoOfFields_String = 0;
static const size_t NoOfFields_Symbol = 0;
static const size_t NoOfFields_Double = 0;
static const size_t NoOfFields_Integer = 0;
static const size_t NoOfFields_Array = NoOfFields_Object;
static const size_t NoOfFields_Invokable = 2;
static const size_t NoOfFields_Method = NoOfFields_Invokable;
static const size_t NoOfFields_Class = 4 + NoOfFields_Object;
static const size_t NoOfFields_Frame = 3 + NoOfFields_Array;
static const size_t NoOfFields_Block = 2 + NoOfFields_Object;
static const size_t NoOfFields_Primitive = NoOfFields_Invokable;
static const size_t NoOfFields_EvaluationPrimitive = NoOfFields_Invokable;

static vector<gc_oop_t> walkedObjects;
/*
 * This method simply pushes all objects into the vector walkedObjects
 */
static gc_oop_t collectMembers(gc_oop_t obj) {
    walkedObjects.push_back(obj);
    return obj;
}
/*
 * Helper function that searches the result vector for a field
 */
static bool WalkerHasFound(gc_oop_t obj) {
    return find(walkedObjects.begin(), walkedObjects.end(), obj) !=
           walkedObjects.end();
}

void WalkObjectsTest::testWalkInteger() {
    walkedObjects.clear();
    VMInteger* int1 = Universe::NewInteger(42);
    int1->WalkObjects(collectMembers);

    // Integers have no additional members
    CPPUNIT_ASSERT_EQUAL(NoOfFields_Integer, walkedObjects.size());
}

void WalkObjectsTest::testWalkDouble() {
    walkedObjects.clear();
    VMDouble* d1 = Universe::NewDouble(432.1);
    d1->WalkObjects(collectMembers);

    // Doubles have no additional members
    CPPUNIT_ASSERT_EQUAL(NoOfFields_Double, walkedObjects.size());
}

void WalkObjectsTest::testWalkEvaluationPrimitive() {
    walkedObjects.clear();

    auto* evPrim = new (GetHeap<HEAP_CLS>(), 0) VMEvaluationPrimitive(1);
    evPrim->SetHolder(load_ptr(classClass));
    evPrim->WalkObjects(collectMembers);

    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(evPrim->GetSignature())));
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(evPrim->GetHolder())));
    CPPUNIT_ASSERT_EQUAL(NoOfFields_EvaluationPrimitive, walkedObjects.size());
}

void WalkObjectsTest::testWalkObject() {
    walkedObjects.clear();

    auto* obj = new (GetHeap<HEAP_CLS>(), 0) VMObject(0, sizeof(VMObject));
    obj->WalkObjects(collectMembers);

    // Objects should only have one member -> Class
    CPPUNIT_ASSERT_EQUAL(NoOfFields_Object, walkedObjects.size());
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(obj->GetClass())));
}

void WalkObjectsTest::testWalkString() {
    walkedObjects.clear();
    VMString* str1 = Universe::NewString("str1");
    str1->WalkObjects(collectMembers);

    CPPUNIT_ASSERT_EQUAL(NoOfFields_String, walkedObjects.size());
}

void WalkObjectsTest::testWalkSymbol() {
    walkedObjects.clear();
    VMSymbol* sym = NewSymbol("symbol");
    sym->WalkObjects(collectMembers);

    CPPUNIT_ASSERT_EQUAL(NoOfFields_Symbol, walkedObjects.size());
}

void WalkObjectsTest::testWalkClass() {
    walkedObjects.clear();
    VMClass* meta = Universe::NewMetaclassClass();
    meta->superClass = stringClass;
    meta->WalkObjects(collectMembers);

    // Now check if we found all class fields
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(meta->GetClass())));
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(meta->GetSuperClass())));
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(meta->GetName())));
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(meta->GetInstanceFields())));
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(meta->GetInstanceInvokables())));
    CPPUNIT_ASSERT_EQUAL(NoOfFields_Class, walkedObjects.size());
}

void WalkObjectsTest::testWalkPrimitive() {
    walkedObjects.clear();
    VMSymbol* primitiveSymbol = NewSymbol("myPrimitive");
    VMInvokable* prim = VMPrimitive::GetEmptyPrimitive(primitiveSymbol, false);
    prim->SetHolder(load_ptr(methodClass));

    prim->WalkObjects(collectMembers);
    CPPUNIT_ASSERT_EQUAL(NoOfFields_Primitive, walkedObjects.size());
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(prim->GetSignature())));
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(prim->GetHolder())));
}

void WalkObjectsTest::testWalkFrame() {
    walkedObjects.clear();
    VMSymbol* methodSymbol = NewSymbol("frameMethod");

    vector<BackJump> inlinedLoops;
    VMMethod* method =
        Universe::NewMethod(methodSymbol, 0, 0, 0, 0,
                            new LexicalScope(nullptr, {}, {}), inlinedLoops);

    VMFrame* prev = Universe::NewFrame(nullptr, method);
    VMFrame* frame = Universe::NewFrame(prev, method);
    frame->SetContext(frame->CloneForMovingGC());
    VMInteger* dummyArg = Universe::NewInteger(1111);
    frame->SetArgument(0, 0, dummyArg);
    frame->WalkObjects(collectMembers);

    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(frame->GetPreviousFrame())));
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(frame->GetContext())));
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(frame->GetMethod())));
    // CPPUNIT_ASSERT(WalkerHasFound(frame->bytecodeIndex));
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(dummyArg)));
    CPPUNIT_ASSERT_EQUAL(
        (size_t)(NoOfFields_Frame + method->GetNumberOfArguments()),
        (size_t)walkedObjects.size() +
            1);  // + 1 for the class field that's still in there
}

static Variable makeVar(const char* const name, bool isArgument) {
    std::string n = name;
    return {n, 0, isArgument, {0, 0}};
}

void WalkObjectsTest::testWalkMethod() {
    walkedObjects.clear();
    // First, we're setting up lexical scopes, just to see that we reach those,
    // too

    vector<Variable> argsInner;
    vector<Variable> localsInner;

    vector<Variable> args;
    vector<Variable> locals;

    argsInner.push_back(makeVar("argInner1", true));
    argsInner.push_back(makeVar("argInner2", true));
    args.push_back(makeVar("arg1", true));
    args.push_back(makeVar("arg2", true));

    localsInner.push_back(makeVar("localInner1", false));
    localsInner.push_back(makeVar("localInner2", false));
    locals.push_back(makeVar("local1", false));
    locals.push_back(makeVar("local2", false));

    auto* inner = new LexicalScope(nullptr, argsInner, localsInner);
    auto* scope = new LexicalScope(inner, args, locals);

    VMSymbol* methodSymbol = NewSymbol("myMethod");

    vector<BackJump> inlinedLoops;
    VMMethod* method =
        Universe::NewMethod(methodSymbol, 0, 0, 0, 0, scope, inlinedLoops);

    method->SetHolder(load_ptr(symbolClass));
    method->WalkObjects(collectMembers);

    // the following fields had no getters -> had to become friend
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(method->GetHolder())));
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(method->GetSignature())));

    const size_t expectedNumberOfObjects = NoOfFields_Method;
    CPPUNIT_ASSERT_EQUAL(expectedNumberOfObjects, walkedObjects.size());
}

void WalkObjectsTest::testWalkBlock() {
    walkedObjects.clear();
    VMSymbol* methodSymbol = NewSymbol("someMethod");

    vector<BackJump> inlinedLoops;
    VMMethod* method =
        Universe::NewMethod(methodSymbol, 0, 0, 0, 0,
                            new LexicalScope(nullptr, {}, {}), inlinedLoops);

    VMBlock* block = Universe::NewBlock(method, Interpreter::GetFrame(),
                                        method->GetNumberOfArguments());
    block->WalkObjects(collectMembers);
    CPPUNIT_ASSERT_EQUAL(NoOfFields_Block, walkedObjects.size());
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(block->GetClass())));
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(block->GetContext())));
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(method)));
}

void WalkObjectsTest::testWalkArray() {
    walkedObjects.clear();
    VMString* str1 = Universe::NewString("str1");
    VMInteger* int1 = Universe::NewInteger(42);
    VMArray* a = Universe::NewArray(2);
    a->SetIndexableField(0, str1);
    a->SetIndexableField(1, int1);
    a->WalkObjects(collectMembers);

    CPPUNIT_ASSERT_EQUAL(NoOfFields_Array + 2, walkedObjects.size());
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(a->GetClass())));
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(str1)));
    CPPUNIT_ASSERT(WalkerHasFound(tmp_ptr(int1)));
}
