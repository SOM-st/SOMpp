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

#include <map>
#include <vector>

#include "../interpreter/Interpreter.h"
#include "../memory/Heap.h"
#include "../misc/Timer.h"
#include "../misc/defs.h"
#include "../vmobjects/ObjectFormats.h"
#include "Print.h"

class SourcecodeCompiler;

// for runtime debug
extern uint8_t dumpBytecodes;
extern uint8_t gcVerbosity;
extern bool abortOnCoreLibHashMismatch;

using namespace std;
class Universe {
public:
    // static methods
    static void Start(int32_t argc, char** argv);
    static void BasicInit();

    static vm_oop_t interpret(const std::string& className,
                              const std::string& methodName);

    static void setupClassPath(const std::string& cp);

    static void Assert(bool /*value*/);

    // VMObject instanciation methods. These should probably be refactored to a
    // new class
    static VMArray* NewArray(size_t /*size*/);
    static VMArray* NewExpandedArrayFromArray(size_t size, VMArray* array);
    static VMVector* NewVector(size_t /*size*/, VMClass* cls);

    static VMArray* NewArrayList(std::vector<vm_oop_t>& list);
    static VMArray* NewArrayList(std::vector<VMInvokable*>& list);
    static VMArray* NewArrayList(std::vector<VMSymbol*>& list);

    static VMArray* NewArrayFromStrings(const vector<std::string>& /*strings*/);
    static VMArray* NewArrayOfSymbolsFromStrings(
        const vector<std::string>& /*strings*/);

    static VMBlock* NewBlock(VMInvokable* method, VMFrame* context,
                             uint8_t arguments);
    static VMClass* NewClass(VMClass* /*classOfClass*/);
    static VMFrame* NewFrame(VMFrame* /*previousFrame*/, VMMethod* /*method*/);
    static VMMethod* NewMethod(VMSymbol* /*signature*/,
                               size_t numberOfBytecodes,
                               size_t numberOfConstants, size_t numLocals,
                               size_t maxStackDepth,
                               LexicalScope* /*lexicalScope*/,
                               vector<BackJump>& inlinedLoops);
    static VMObject* NewInstance(VMClass* /*classOfInstance*/);
    static VMObject* NewInstanceWithoutFields();
    static VMInteger* NewInteger(int64_t /*value*/);
    static void WalkGlobals(walk_heap_fn /*walk*/);
    static VMDouble* NewDouble(double /*value*/);
    static VMClass* NewMetaclassClass();
    static VMString* NewString(const std::string& str);
    static VMString* NewString(size_t length, const char* str);
    static VMClass* NewSystemClass();

    static void InitializeSystemClass(VMClass* /*systemClass*/,
                                      VMClass* /*superClass*/,
                                      const char* /*name*/);

    static vm_oop_t GetGlobal(VMSymbol* /*name*/);
    static void SetGlobal(VMSymbol* name, vm_oop_t val);
    static bool HasGlobal(VMSymbol* /*name*/);
    static VMObject* InitializeGlobals();
    static VMClass* GetBlockClass();
    static VMClass* GetBlockClassWithArgs(uint8_t numberOfArguments);

    static VMClass* LoadClass(VMSymbol* /*name*/);
    static void LoadSystemClass(VMClass* /*systemClass*/);
    static VMClass* LoadClassBasic(VMSymbol* /*name*/,
                                   VMClass* /*systemClass*/);
    static VMClass* LoadShellClass(std::string& /*stmt*/);

    Universe() = default;
    ~Universe();
#ifdef LOG_RECEIVER_TYPES
    struct stat_data {
        long noCalls;
        long noPrimitiveCalls;
    };
    static map<std::string, long> receiverTypes;
    static map<std::string, stat_data> callStats;
#endif
    //

    static void Shutdown();

private:
    static vm_oop_t interpretMethod(VMObject* receiver, VMInvokable* initialize,
                                    VMArray* argumentsArray);

    static vector<std::string> handleArguments(int32_t argc, char** argv);
    static bool getClassPathExt(vector<std::string>& tokens,
                                const std::string& arg);

    static VMMethod* createBootstrapMethod(VMClass* holder,
                                           uint8_t numArgsOfMsgSend);

    static void addClassPath(const std::string& cp);
    static void printUsageAndExit(char* executable);

    static void initialize(int32_t _argc, char** _argv);

    static size_t heapSize;
    static map<GCSymbol*, gc_oop_t> globals;

    static map<uint8_t, GCClass*> blockClassesByNoOfArgs;
    static vector<std::string> classPath;
};
