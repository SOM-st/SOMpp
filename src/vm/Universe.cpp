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

#include "Universe.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "../compiler/Disassembler.h"
#include "../compiler/LexicalScope.h"
#include "../compiler/SourcecodeCompiler.h"
#include "../interpreter/bytecodes.h"
#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../vmobjects/IntegerBox.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMArray.h"
#include "../vmobjects/VMBlock.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMDouble.h"
#include "../vmobjects/VMEvaluationPrimitive.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMInteger.h"
#include "../vmobjects/VMMethod.h"
#include "../vmobjects/VMObject.h"
#include "../vmobjects/VMObjectBase.h"
#include "../vmobjects/VMString.h"
#include "../vmobjects/VMVector.h"
#include "Globals.h"
#include "IsValidObject.h"
#include "LogAllocation.h"
#include "Print.h"
#include "Shell.h"
#include "Symbols.h"

#if CACHE_INTEGER
static gc_oop_t prebuildInts[INT_CACHE_MAX_VALUE - INT_CACHE_MIN_VALUE + 1];
#endif

#define INT_HIST_SIZE 1

// Here we go:

uint8_t dumpBytecodes;
uint8_t gcVerbosity;

static std::string bm_name;

static map<int64_t, int64_t> integerHist;

map<GCSymbol*, gc_oop_t> Universe::globals;
map<uint8_t, GCClass*> Universe::blockClassesByNoOfArgs;
vector<std::string> Universe::classPath;
size_t Universe::heapSize;

void Universe::Start(int32_t argc, char** argv) {
    BasicInit();
    Universe::initialize(argc, argv);
}

void Universe::BasicInit() {
    assert(Bytecode::BytecodeDefinitionsAreConsistent());
}

void Universe::Shutdown() {
    if (gcVerbosity > 0) {
        ErrorPrint("Time spent in GC: [" +
                   to_string(Timer::GCTimer.GetTotalTime()) + "] msec\n");
    }

#ifdef GENERATE_INTEGER_HISTOGRAM
    std::string file_name_hist = std::string(bm_name);
    file_name_hist.append("_integer_histogram.csv");
    fstream hist_csv(file_name_hist.c_str(), ios::out);

    for (map<long, long>::iterator it = integerHist.begin();
         it != integerHist.end();
         it++) {
        hist_csv << it->first << ", " << it->second << endl;
    }
#endif

#ifdef LOG_RECEIVER_TYPES
    std::string file_name_receivers = std::string(bm_name);
    file_name_receivers.append("_receivers.csv");
    fstream receivers(file_name_receivers.c_str(), ios::out);
    for (map<std::string, long>::iterator it =
             theUniverse->receiverTypes.begin();
         it != theUniverse->receiverTypes.end();
         it++) {
        receivers << it->first << ",  " << it->second << endl;
    }

    std::string file_name_send_types = std::string(bm_name);
    file_name_send_types.append("_send_types.csv");
    fstream send_stat(file_name_send_types.c_str(), ios::out);
    send_stat << "#name, percentage_primitive_calls, no_primitive_calls, "
                 "no_non_primitive_calls"
              << endl;
    for (map<std::string, Universe::stat_data>::iterator it =
             theUniverse->callStats.begin();
         it != theUniverse->callStats.end();
         it++) {
        send_stat << it->first << ", " << setiosflags(ios::fixed)
                  << setprecision(2)
                  << (double)(it->second.noPrimitiveCalls) /
                         (double)(it->second.noCalls)
                  << ", " << it->second.noPrimitiveCalls << ", "
                  << it->second.noCalls - it->second.noPrimitiveCalls << endl;
    }
#endif
}

static void printVmConfig() {
    if (GC_TYPE == GENERATIONAL) {
        cout << "\tgarbage collector: generational\n";
    } else if (GC_TYPE == COPYING) {
        cout << "\tgarbage collector: copying\n";
    } else if (GC_TYPE == MARK_SWEEP) {
        cout << "\tgarbage collector: mark-sweep\n";
    } else if (GC_TYPE == DEBUG_COPYING) {
        cout << "\tgarbage collector: debug copying\n";
    } else {
        cout << "\tgarbage collector: unknown\n";
    }

    if (USE_TAGGING) {
        cout << "\twith tagged integers\n";
    } else {
        cout << "\tnot tagging integers\n";
    }

    if (CACHE_INTEGER) {
        cout << "\tcaching integers from " << INT_CACHE_MIN_VALUE << " to "
             << INT_CACHE_MAX_VALUE << "\n";
    } else {
        cout << "\tnot caching integers\n";
    }

    if (USE_VECTOR_PRIMITIVES) {
        cout << "\tVector primitives: enabled\n";
    } else {
        cout << "\tVector primitives: disabled\n";
    }

    cout << "--------------------------------------\n";
}

vector<std::string> Universe::handleArguments(int32_t argc, char** argv) {
    vector<std::string> vmArgs = vector<std::string>();
    dumpBytecodes = 0;
    gcVerbosity = 0;

    for (int32_t i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "-cp", 3) == 0) {
            if ((argc == i + 1) || !classPath.empty()) {
                printUsageAndExit(argv[0]);
            }
            setupClassPath(std::string(argv[++i]));
        } else if (strncmp(argv[i], "-d", 2) == 0) {
            ++dumpBytecodes;
        } else if (strncmp(argv[i], "-cfg", 4) == 0) {
            printVmConfig();
        } else if (strncmp(argv[i], "-g", 2) == 0) {
            ++gcVerbosity;
        } else if (strncmp(argv[i], "-H", 2) == 0) {
            size_t heap_size = 0;
            char unit[3];
            // NOLINTNEXTLINE (cert-err34-c)
            if (sscanf(argv[i], "-H%ld%2s", &heap_size, unit) == 2) {
                if (strcmp(unit, "KB") == 0) {
                    heapSize = heap_size * 1024;
                } else if (strcmp(unit, "MB") == 0) {
                    heapSize = heap_size * 1024 * 1024;
                }
            } else {
                printUsageAndExit(argv[0]);
            }

        } else if ((strncmp(argv[i], "-h", 2) == 0) ||
                   (strncmp(argv[i], "--help", 6) == 0)) {
            printUsageAndExit(argv[0]);
        } else {
            vector<std::string> extPathTokens = vector<std::string>(2);
            std::string const tmpString = std::string(argv[i]);
            if (getClassPathExt(extPathTokens, tmpString)) {
                addClassPath(extPathTokens[0]);
            }
            // Different from CSOM!!!:
            // In CSOM there is an else, where the original filename is pushed
            // into the vm_args. But unlike the class name in extPathTokens
            // (extPathTokens[1]) that could still have the .som suffix though.
            // So in SOM++ getClassPathExt will strip the suffix and add it to
            // extPathTokens even if there is no new class path present. So we
            // can in any case do the following:
            vmArgs.push_back(extPathTokens[1]);
        }
    }
    addClassPath(std::string("."));

    return vmArgs;
}

bool Universe::getClassPathExt(vector<std::string>& tokens,
                               const std::string& arg) {
#define EXT_TOKENS 2
    bool result = true;
    size_t fpIndex = arg.find_last_of(fileSeparator);
    size_t ssepIndex = arg.find(".som");

    if (fpIndex == std::string::npos) {  // no new path
        // different from CSOM (see also HandleArguments):
        // we still want to strip the suffix from the filename, so
        // we set the start to -1, in order to start the substring
        // from character 0. npos is -1 too, but this is to make sure
        fpIndex = -1;
        // instead of returning here directly, we have to remember that
        // there is no new class path and return it later
        result = false;
    } else {
        tokens[0] = arg.substr(0, fpIndex);
    }

    // adding filename (minus ".som" if present) to second slot
    ssepIndex = ((ssepIndex != std::string::npos) && (ssepIndex > fpIndex))
                    ? (ssepIndex - 1)
                    : arg.length();
    tokens[1] = arg.substr(fpIndex + 1, ssepIndex - (fpIndex));
    return result;
}

void Universe::setupClassPath(const std::string& cp) {
    std::stringstream ss(cp);
    std::string token;

    while (getline(ss, token, pathSeparator)) {
        classPath.push_back(token);
    }
}

void Universe::addClassPath(const std::string& cp) {
    classPath.push_back(cp);
}

void Universe::printUsageAndExit(char* executable) {
    cout << "Usage: " << executable << " [-options] [args...]\n\n";
    cout << "where options include:\n";
    cout << "    -cp  <directories separated by " << pathSeparator << ">\n";
    cout << "         set search path for application classes\n";
    cout << "    -d   enable disassembling (twice for tracing)\n";
    cout << "    -cfg print VM configuration\n";
    cout
        << "    -g   enable garbage collection details:\n"
        << "         1x - print statistics when VM shuts down\n"
        << "         2x - print statistics upon each collection\n"
        << "         3x - print statistics and dump heap upon each collection\n"
        << "\n";
    cout << "    -HxMB set the heap size to x MB (default: 1 MB)\n";
    cout << "    -HxKB set the heap size to x KB (default: 1 MB)\n";
    cout << "    -h  show this help\n";

    Quit(ERR_SUCCESS);
}

VMMethod* Universe::createBootstrapMethod(VMClass* holder,
                                          uint8_t numArgsOfMsgSend) {
    vector<BackJump> inlinedLoops;
    auto* bootStrapScope = new LexicalScope(nullptr, {}, {});
    VMMethod* bootstrapMethod =
        NewMethod(SymbolFor("bootstrap"), 1, 0, 0, numArgsOfMsgSend,
                  bootStrapScope, inlinedLoops);

    bootstrapMethod->SetBytecode(0, BC_HALT);
    bootstrapMethod->SetHolder(holder);
    return bootstrapMethod;
}

vm_oop_t Universe::interpret(const std::string& className,
                             const std::string& methodName) {
    // This method assumes that SOM++ was already initialized by executing a
    // Hello World program as part of the unittest main.

    bm_name = "BasicInterpreterTests";

    VMSymbol* classNameSym = SymbolFor(className);
    VMClass* clazz = LoadClass(classNameSym);

    // Lookup the method to be executed on the class
    auto* initialize =
        (VMMethod*)clazz->GetClass()->LookupInvokable(SymbolFor(methodName));

    if (initialize == nullptr) {
        ErrorPrint("Lookup of " + className + ">>#" + methodName + " failed");
        return nullptr;
    }

    return interpretMethod(clazz, initialize, nullptr);
}

vm_oop_t Universe::interpretMethod(VMObject* receiver, VMInvokable* initialize,
                                   VMArray* argumentsArray) {
    /* only trace bootstrap if the number of cmd-line "-d"s is > 2 */
    uint8_t const trace = 2 - dumpBytecodes;
    if (!(trace > 0)) {
        dumpBytecodes = 1;
    }

    VMMethod* bootstrapMethod = createBootstrapMethod(load_ptr(systemClass), 2);

    VMFrame* bootstrapFrame = Interpreter::PushNewFrame(bootstrapMethod);
    for (size_t argIdx = 0; argIdx < bootstrapMethod->GetNumberOfArguments();
         argIdx += 1) {
        bootstrapFrame->SetArgument(argIdx, 0, load_ptr(nilObject));
    }

    bootstrapFrame->Push(receiver);

    if (argumentsArray != nullptr) {
        bootstrapFrame->Push(argumentsArray);
    }

    initialize->Invoke(bootstrapFrame);

    // reset "-d" indicator
    if (!(trace > 0)) {
        dumpBytecodes = 2 - trace;
    }

    if (dumpBytecodes > 1) {
        return Interpreter::Start<true>();
    }
    return Interpreter::Start<false>();
}

void Universe::initialize(int32_t _argc, char** _argv) {
    InitializeAllocationLog();

    heapSize = 1ULL * 1024 * 1024;

    vector<std::string> argv = handleArguments(_argc, _argv);

    // remember file that was executed (for writing statistics)
    if (!argv.empty()) {
        bm_name = argv[0];
    }

    Heap<HEAP_CLS>::InitializeHeap(heapSize);

#if CACHE_INTEGER
    // create prebuilt integers
    for (int64_t it = INT_CACHE_MIN_VALUE; it <= INT_CACHE_MAX_VALUE; ++it) {
        prebuildInts[(size_t)(it - INT_CACHE_MIN_VALUE)] =
            store_root(new (GetHeap<HEAP_CLS>(), 0) VMInteger(it));
    }
#endif

    VMObject* systemObject = InitializeGlobals();

    if (argv.empty()) {
        VMMethod* bootstrapMethod =
            createBootstrapMethod(load_ptr(systemClass), 2);
        auto* shell = new Shell(bootstrapMethod);
        shell->Start();
        return;
    }

    VMInvokable* initialize =
        load_ptr(systemClass)->LookupInvokable(SymbolFor("initialize:"));

    VMArray* argumentsArray = NewArrayFromStrings(argv);
    interpretMethod(systemObject, initialize, argumentsArray);
}

Universe::~Universe() {
    // check done inside
    Heap<HEAP_CLS>::DestroyHeap();
}

VMObject* Universe::InitializeGlobals() {
    set_vt_to_null();

    //
    // allocate nil object
    //
    VMObject* nil = NewInstanceWithoutFields();
    nilObject = store_root(nil);

    trueObject = store_root(NewInstanceWithoutFields());
    falseObject = store_root(NewInstanceWithoutFields());

    metaClassClass = store_root(NewMetaclassClass());

    objectClass = store_root(NewSystemClass());
    nilClass = store_root(NewSystemClass());
    classClass = store_root(NewSystemClass());
    arrayClass = store_root(NewSystemClass());
    vectorClass = store_root(NewSystemClass());
    symbolClass = store_root(NewSystemClass());
    methodClass = store_root(NewSystemClass());
    integerClass = store_root(NewSystemClass());
    primitiveClass = store_root(NewSystemClass());
    stringClass = store_root(NewSystemClass());
    doubleClass = store_root(NewSystemClass());

    nil->SetClass(load_ptr(nilClass));

    InitializeSystemClass(load_ptr(objectClass), nullptr, "Object");
    InitializeSystemClass(load_ptr(classClass), load_ptr(objectClass), "Class");
    InitializeSystemClass(load_ptr(metaClassClass), load_ptr(classClass),
                          "Metaclass");
    InitializeSystemClass(load_ptr(nilClass), load_ptr(objectClass), "Nil");
    InitializeSystemClass(load_ptr(arrayClass), load_ptr(objectClass), "Array");
    InitializeSystemClass(load_ptr(vectorClass), load_ptr(objectClass),
                          "Vector");
    InitializeSystemClass(load_ptr(methodClass), load_ptr(arrayClass),
                          "Method");
    InitializeSystemClass(load_ptr(stringClass), load_ptr(objectClass),
                          "String");
    InitializeSystemClass(load_ptr(symbolClass), load_ptr(stringClass),
                          "Symbol");
    InitializeSystemClass(load_ptr(integerClass), load_ptr(objectClass),
                          "Integer");
    InitializeSystemClass(load_ptr(primitiveClass), load_ptr(objectClass),
                          "Primitive");
    InitializeSystemClass(load_ptr(doubleClass), load_ptr(objectClass),
                          "Double");

    // Fix up objectClass
    load_ptr(objectClass)->SetSuperClass(nil);

#if USE_TAGGING
    GlobalBox::updateIntegerBox(NewInteger(1));
#endif
    InitializeSymbols();
    obtain_vtables_of_known_classes(load_ptr(symbolSelf));

    LoadSystemClass(load_ptr(objectClass));
    LoadSystemClass(load_ptr(classClass));
    LoadSystemClass(load_ptr(metaClassClass));
    LoadSystemClass(load_ptr(nilClass));
    LoadSystemClass(load_ptr(arrayClass));
    LoadSystemClass(load_ptr(vectorClass));
    LoadSystemClass(load_ptr(methodClass));
    LoadSystemClass(load_ptr(symbolClass));
    LoadSystemClass(load_ptr(integerClass));
    LoadSystemClass(load_ptr(primitiveClass));
    LoadSystemClass(load_ptr(stringClass));
    LoadSystemClass(load_ptr(doubleClass));

    blockClass = store_root(LoadClass(SymbolFor("Block")));

    VMSymbol* trueClassName = SymbolFor("True");
    trueClass = store_root(LoadClass(trueClassName));
    load_ptr(trueObject)->SetClass(load_ptr(trueClass));

    VMSymbol* falseClassName = SymbolFor("False");
    falseClass = store_root(LoadClass(falseClassName));
    load_ptr(falseObject)->SetClass(load_ptr(falseClass));

    systemClass = store_root(LoadClass(SymbolFor("System")));

    VMObject* systemObject = NewInstance(load_ptr(systemClass));

    SetGlobal(SymbolFor("nil"), load_ptr(nilObject));
    SetGlobal(SymbolFor("true"), load_ptr(trueObject));
    SetGlobal(SymbolFor("false"), load_ptr(falseObject));
    SetGlobal(SymbolFor("system"), systemObject);
    SetGlobal(SymbolFor("System"), load_ptr(systemClass));
    SetGlobal(SymbolFor("Block"), load_ptr(blockClass));

    return systemObject;
}

void Universe::Assert(bool value) {
    if (!value) {
        ErrorPrint("Universe::Assert Assertion failed\n");
    }
}

VMClass* Universe::GetBlockClass() {
    return load_ptr(blockClass);
}

VMClass* Universe::GetBlockClassWithArgs(uint8_t numberOfArguments) {
    auto const it = blockClassesByNoOfArgs.find(numberOfArguments);
    if (it != blockClassesByNoOfArgs.end()) {
        return load_ptr(it->second);
    }

    Assert(numberOfArguments < 10);

    ostringstream Str;
    Str << "Block" << to_string(numberOfArguments);
    VMSymbol* name = SymbolFor(Str.str());
    VMClass* result = LoadClassBasic(name, nullptr);

    result->InstallPrimitive(new (GetHeap<HEAP_CLS>(), 0)
                                 VMEvaluationPrimitive(numberOfArguments),
                             0, false);

    SetGlobal(name, result);
    blockClassesByNoOfArgs[numberOfArguments] = store_root(result);

    return result;
}

vm_oop_t Universe::GetGlobal(VMSymbol* name) {
    auto it = globals.find(tmp_ptr(name));
    if (it == globals.end()) {
        return nullptr;
    }
    return load_ptr(it->second);
}

bool Universe::HasGlobal(VMSymbol* name) {
    auto it = globals.find(tmp_ptr(name));
    return it != globals.end();
}

void Universe::InitializeSystemClass(VMClass* systemClass, VMClass* superClass,
                                     const char* name) {
    std::string const s_name(name);

    if (superClass != nullptr) {
        systemClass->SetSuperClass(superClass);
        VMClass* sysClassClass = systemClass->GetClass();
        VMClass* superClassClass = superClass->GetClass();
        sysClassClass->SetSuperClass(superClassClass);
    } else {
        VMClass* sysClassClass = systemClass->GetClass();
        sysClassClass->SetSuperClass(load_ptr(classClass));
    }

    VMClass* sysClassClass = systemClass->GetClass();

    systemClass->SetInstanceFields(NewArray(0));
    sysClassClass->SetInstanceFields(NewArray(0));

    systemClass->SetInstanceInvokables(NewArray(0));
    sysClassClass->SetInstanceInvokables(NewArray(0));

    systemClass->SetName(SymbolFor(s_name));
    ostringstream Str;
    Str << s_name << " class";
    std::string const classClassName(Str.str());
    sysClassClass->SetName(SymbolFor(classClassName));

    SetGlobal(systemClass->GetName(), systemClass);
}

VMClass* Universe::LoadClass(VMSymbol* name) {
    auto* result = static_cast<VMClass*>(GetGlobal(name));

    if (result != nullptr) {
        return result;
    }

    result = LoadClassBasic(name, nullptr);

    if (result == nullptr) {
        // we fail silently, it is not fatal that loading a class failed
        return (VMClass*)nilObject;
    }

    if (result->HasPrimitives() || result->GetClass()->HasPrimitives()) {
        result->LoadPrimitives(true);
    }

    SetGlobal(name, result);

    return result;
}

VMClass* Universe::LoadClassBasic(VMSymbol* name, VMClass* systemClass) {
    std::string const sName = name->GetStdString();
    VMClass* result = nullptr;

    for (auto& i : classPath) {
        result = SourcecodeCompiler::CompileClass(i, sName, systemClass);
        if (result != nullptr) {
            if (dumpBytecodes != 0) {
                Disassembler::Dump(result->GetClass());
                Disassembler::Dump(result);
            }

            return result;
        }
    }
    return nullptr;
}

VMClass* Universe::LoadShellClass(std::string& stmt) {
    VMClass* result = SourcecodeCompiler::CompileClassString(stmt, nullptr);
    if (dumpBytecodes != 0) {
        Disassembler::Dump(result);
    }
    return result;
}

void Universe::LoadSystemClass(VMClass* systemClass) {
    VMClass* result = LoadClassBasic(systemClass->GetName(), systemClass);
    std::string const s = systemClass->GetName()->GetStdString();

    if (result == nullptr) {
        ErrorPrint("Can't load system class: " + s + "\n");
        Quit(ERR_FAIL);
    }

    // Vector has primitive methods and should be loaded (This is temporary)
    if (result->HasPrimitives() || result->GetClass()->HasPrimitives() ||
        (result->GetName()->GetStdString() == "Vector" &&
         USE_VECTOR_PRIMITIVES == true)) {
        result->LoadPrimitives(false);
    }
}

// Should create a new instance of Vector
VMVector* Universe::NewVector(size_t size, VMClass* cls) {
    vm_oop_t first = NEW_INT(1);
    vm_oop_t last = NEW_INT(1);
    auto* storageArray = NewArray(size);
    auto* result =
        new (GetHeap<HEAP_CLS>(), 0) VMVector(first, last, storageArray);
    result->SetClass(cls);
    LOG_ALLOCATION("VMVector", result->GetObjectSize());
    return result;
}

VMArray* Universe::NewArray(size_t size) {
    size_t const additionalBytes = size * sizeof(VMObject*);

    bool outsideNursery = false;  // NOLINT

#if GC_TYPE == GENERATIONAL
    // if the array is too big for the nursery, we will directly allocate a
    // mature object
    outsideNursery = additionalBytes + sizeof(VMArray) >
                     GetHeap<HEAP_CLS>()->GetMaxNurseryObjectSize();
#endif

    auto* result = new (GetHeap<HEAP_CLS>(),
                        additionalBytes ALLOC_OUTSIDE_NURSERY(outsideNursery))
        VMArray(size, additionalBytes);
    if ((GC_TYPE == GENERATIONAL) && outsideNursery) {
        result->SetGCField(MASK_OBJECT_IS_OLD);
    }

    result->SetClass(load_ptr(arrayClass));

    LOG_ALLOCATION("VMArray", result->GetObjectSize());
    return result;
}

VMArray* Universe::NewExpandedArrayFromArray(size_t size, VMArray* array) {
    size_t const additionalBytes = size * sizeof(VMObject*);

    bool outsideNursery = false;  // NOLINT

    size_t const currentArraySize = array->GetNumberOfIndexableFields();

#if GC_TYPE == GENERATIONAL
    // if the array is too big for the nursery, we will directly allocate a
    // mature object
    outsideNursery = additionalBytes + sizeof(VMArray) >
                     GetHeap<HEAP_CLS>()->GetMaxNurseryObjectSize();
#endif

    auto* result = new (GetHeap<HEAP_CLS>(),
                        additionalBytes ALLOC_OUTSIDE_NURSERY(outsideNursery))
        VMArray(size, additionalBytes, currentArraySize);
    if ((GC_TYPE == GENERATIONAL) && outsideNursery) {
        result->SetGCField(MASK_OBJECT_IS_OLD);
    }

    result->SetClass(load_ptr(arrayClass));

    LOG_ALLOCATION("VMArray", result->GetObjectSize());

    // Now copy the contents of the old array into the new one
    for (size_t i = 0; i < currentArraySize; ++i) {
        result->SetIndexableField(i, array->GetIndexableField(i));
    }
    return result;
}

VMArray* Universe::NewArrayFromStrings(const vector<std::string>& strings) {
    VMArray* result = NewArray(strings.size());
    size_t j = 0;
    for (const std::string& str : strings) {
        result->SetIndexableField(j, NewString(str));
        j += 1;
    }
    return result;
}

VMArray* Universe::NewArrayOfSymbolsFromStrings(
    const vector<std::string>& strings) {
    VMArray* result = NewArray(strings.size());
    size_t j = 0;
    for (const std::string& str : strings) {
        result->SetIndexableField(j, SymbolFor(str));
        j += 1;
    }
    return result;
}

VMArray* Universe::NewArrayList(std::vector<VMSymbol*>& list) {
    auto& objList = (std::vector<vm_oop_t>&)list;
    return NewArrayList(objList);
}

VMArray* Universe::NewArrayList(std::vector<VMInvokable*>& list) {
    auto& objList = (std::vector<vm_oop_t>&)list;
    return NewArrayList(objList);
}

VMArray* Universe::NewArrayList(std::vector<vm_oop_t>& list) {
    size_t const size = list.size();
    VMArray* result = NewArray(size);

    if (result != nullptr) {
        for (size_t i = 0; i < size; i += 1) {
            vm_oop_t elem = list[i];
            result->SetIndexableField(i, elem);
        }
    }
    return result;
}

VMBlock* Universe::NewBlock(VMInvokable* method, VMFrame* context,
                            uint8_t arguments) {
    auto* result = new (GetHeap<HEAP_CLS>(), 0) VMBlock(method, context);
    result->SetClass(GetBlockClassWithArgs(arguments));

    LOG_ALLOCATION("VMBlock", result->GetObjectSize());
    return result;
}

VMClass* Universe::NewClass(VMClass* classOfClass) {
    size_t const numFields = classOfClass->GetNumberOfInstanceFields();
    VMClass* result = nullptr;

    if (numFields != 0U) {
        size_t const additionalBytes = numFields * sizeof(VMObject*);
        result = new (GetHeap<HEAP_CLS>(), additionalBytes)
            VMClass(numFields, additionalBytes);
    } else {
        result = new (GetHeap<HEAP_CLS>(), 0) VMClass;
    }

    result->SetClass(classOfClass);

    LOG_ALLOCATION("VMClass", result->GetObjectSize());
    return result;
}

VMDouble* Universe::NewDouble(double value) {
    LOG_ALLOCATION("VMDouble", sizeof(VMDouble));
    return new (GetHeap<HEAP_CLS>(), 0) VMDouble(value);
}

VMFrame* Universe::NewFrame(VMFrame* previousFrame, VMMethod* method) {
    VMFrame* result = nullptr;
#ifdef UNSAFE_FRAME_OPTIMIZATION
    result = method->GetCachedFrame();
    if (result != nullptr) {
        method->SetCachedFrame(nullptr);
        result->SetPreviousFrame(previousFrame);
        return result;
    }
#endif
    size_t const length = method->GetNumberOfArguments() +
                          method->GetNumberOfLocals() +
                          method->GetMaximumNumberOfStackElements();

    size_t const additionalBytes = length * sizeof(VMObject*);
    result = new (GetHeap<HEAP_CLS>(), additionalBytes)
        VMFrame(additionalBytes, method, previousFrame);

    LOG_ALLOCATION("VMFrame", result->GetObjectSize());
    return result;
}

VMObject* Universe::NewInstance(VMClass* classOfInstance) {
    size_t const numOfFields = classOfInstance->GetNumberOfInstanceFields();
    // the additional space needed is calculated from the number of fields
    size_t const additionalBytes = numOfFields * sizeof(VMObject*);
    auto* result = new (GetHeap<HEAP_CLS>(), additionalBytes)
        VMObject(numOfFields, additionalBytes + sizeof(VMObject));
    result->SetClass(classOfInstance);

    LOG_ALLOCATION(classOfInstance->GetName()->GetStdString(),
                   result->GetObjectSize());
    return result;
}

VMObject* Universe::NewInstanceWithoutFields() {
    auto* result = new (GetHeap<HEAP_CLS>(), 0) VMObject(0, sizeof(VMObject));
    return result;
}

VMInteger* Universe::NewInteger(int64_t value) {
#ifdef GENERATE_INTEGER_HISTOGRAM
    integerHist[value / INT_HIST_SIZE] = integerHist[value / INT_HIST_SIZE] + 1;
#endif

#if CACHE_INTEGER
    size_t const index = (size_t)value - (size_t)INT_CACHE_MIN_VALUE;
    if (index < (size_t)(INT_CACHE_MAX_VALUE - INT_CACHE_MIN_VALUE)) {
        return static_cast<VMInteger*>(load_ptr(prebuildInts[index]));
    }
#endif

    LOG_ALLOCATION("VMInteger", sizeof(VMInteger));
    return new (GetHeap<HEAP_CLS>(), 0) VMInteger(value);
}

VMClass* Universe::NewMetaclassClass() {
    auto* result = new (GetHeap<HEAP_CLS>(), 0) VMClass;
    auto* mclass = new (GetHeap<HEAP_CLS>(), 0) VMClass;
    result->SetClass(mclass);
    mclass->SetClass(result);

    LOG_ALLOCATION("VMClass", result->GetObjectSize());
    return result;
}

void Universe::WalkGlobals(walk_heap_fn walk) {
    nilObject = static_cast<GCObject*>(walk(nilObject));
    trueObject = static_cast<GCObject*>(walk(trueObject));
    falseObject = static_cast<GCObject*>(walk(falseObject));

#if USE_TAGGING
    GlobalBox::WalkGlobals(walk);
#endif

    objectClass = static_cast<GCClass*>(walk(objectClass));
    classClass = static_cast<GCClass*>(walk(classClass));
    metaClassClass = static_cast<GCClass*>(walk(metaClassClass));

    nilClass = static_cast<GCClass*>(walk(nilClass));
    integerClass = static_cast<GCClass*>(walk(integerClass));
    arrayClass = static_cast<GCClass*>(walk(arrayClass));
    vectorClass = static_cast<GCClass*>(walk(vectorClass));
    methodClass = static_cast<GCClass*>(walk(methodClass));
    symbolClass = static_cast<GCClass*>(walk(symbolClass));
    primitiveClass = static_cast<GCClass*>(walk(primitiveClass));
    stringClass = static_cast<GCClass*>(walk(stringClass));
    systemClass = static_cast<GCClass*>(walk(systemClass));
    blockClass = static_cast<GCClass*>(walk(blockClass));
    doubleClass = static_cast<GCClass*>(walk(doubleClass));

    trueClass = static_cast<GCClass*>(walk(trueClass));
    falseClass = static_cast<GCClass*>(walk(falseClass));

#if CACHE_INTEGER
    for (size_t i = 0; i < (INT_CACHE_MAX_VALUE - INT_CACHE_MIN_VALUE); i++) {
  #if USE_TAGGING
        prebuildInts[i] = TAG_INTEGER(INT_CACHE_MIN_VALUE + i);
  #else
        prebuildInts[i] = walk(prebuildInts[i]);
  #endif
    }
#endif

    // walk all entries in globals map
    map<GCSymbol*, gc_oop_t> globs = globals;
    globals.clear();
    map<GCSymbol*, gc_oop_t>::iterator iter;
    for (iter = globs.begin(); iter != globs.end(); iter++) {
        assert(iter->second != nullptr);

        auto* key = static_cast<GCSymbol*>(walk(iter->first));
        gc_oop_t val = walk(iter->second);
        globals[key] = val;
    }

    WalkSymbols(walk);

    map<uint8_t, GCClass*>::iterator bcIter;
    for (bcIter = blockClassesByNoOfArgs.begin();
         bcIter != blockClassesByNoOfArgs.end();
         bcIter++) {
        bcIter->second = static_cast<GCClass*>(walk(bcIter->second));
    }

    Interpreter::WalkGlobals(walk);
}

VMMethod* Universe::NewMethod(VMSymbol* signature, size_t numberOfBytecodes,
                              size_t numberOfConstants, size_t numLocals,
                              size_t maxStackDepth, LexicalScope* lexicalScope,
                              vector<BackJump>& inlinedLoops) {
    assert(lexicalScope != nullptr &&
           "A method is expected to have a lexical scope");

    // turn inlined loops vector into a nullptr terminated array
    BackJump* inlinedLoopsArr = nullptr;
    if (inlinedLoops.empty()) {
        inlinedLoopsArr = nullptr;
    } else {
        inlinedLoopsArr = new BackJump[inlinedLoops.size() + 1];
        size_t i = 0;
        for (; i < inlinedLoops.size(); i += 1) {
            inlinedLoopsArr[i] = inlinedLoops[i];
        }

        inlinedLoopsArr[i] = BackJump();
    }

    // method needs space for the bytecodes and the pointers to the constants
    size_t const additionalBytes = PADDED_SIZE(
        numberOfBytecodes + (numberOfConstants * sizeof(VMObject*)));
    auto* result = new (GetHeap<HEAP_CLS>(), additionalBytes)
        VMMethod(signature, numberOfBytecodes, numberOfConstants, numLocals,
                 maxStackDepth, lexicalScope, inlinedLoopsArr);

    LOG_ALLOCATION("VMMethod", result->GetObjectSize());
    return result;
}

VMString* Universe::NewString(const std::string& str) {
    return NewString(str.length(), str.c_str());
}

VMString* Universe::NewString(const size_t length, const char* str) {
    auto* result =
        new (GetHeap<HEAP_CLS>(), PADDED_SIZE(length)) VMString(length, str);

    LOG_ALLOCATION("VMString", result->GetObjectSize());
    return result;
}

VMClass* Universe::NewSystemClass() {
    auto* systemClass = new (GetHeap<HEAP_CLS>(), 0) VMClass();
    auto* mclass = new (GetHeap<HEAP_CLS>(), 0) VMClass();

    systemClass->SetClass(mclass);
    mclass->SetClass(load_ptr(metaClassClass));

    LOG_ALLOCATION("VMClass", systemClass->GetObjectSize());
    return systemClass;
}

void Universe::SetGlobal(VMSymbol* name, vm_oop_t val) {
    globals[store_root(name)] = store_root(val);
}
