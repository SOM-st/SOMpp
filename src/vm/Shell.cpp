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

#include "Shell.h"

#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMInvokable.h"
#include "../vmobjects/VMMethod.h"
#include "Globals.h"
#include "Print.h"
#include "Symbols.h"
#include "Universe.h"

// maximal length of an input line from the shell
#define INPUT_MAX_SIZE BUFSIZ

// some constants for assembling Smalltalk code
#define SHELL_PREFIX "Shell_Class_"
#define SHELL_PART_1 " = (run: it = ( | tmp | tmp := ("
#define SHELL_PART_2 "). 'it = ' print. ^tmp println) )"

Shell::Shell() : bootstrapMethod(nullptr) {}

Shell::Shell(VMMethod* bsm) : bootstrapMethod(bsm) {}

void Shell::Start() {
#define QUIT_CMD "system exit"
#define QUIT_CMD_L (11 + 1)

    if (bootstrapMethod == nullptr) {
        ErrorExit("Shell needs bootstrap method!");
    }
    // the statement to evaluate
    char inbuf[INPUT_MAX_SIZE];
    size_t bytecodeIndex = 0;
    size_t counter = 0;
    VMFrame* currentFrame = nullptr;
    VMClass* runClass = nullptr;
    vm_oop_t it = load_ptr(nilObject);  // last evaluation result.

    cout << "SOM Shell. Type \"" << QUIT_CMD << "\" to exit.\n";

    // Create a fake bootstrap frame
    currentFrame = Interpreter::PushNewFrame(GetBootstrapMethod());
    // Remember the first bytecode index, e.g. index of the halt instruction
    bytecodeIndex = currentFrame->GetBytecodeIndex();

    /**
     * Main Shell Loop
     */
    while (!cin.eof()) {
        // initialize empty strings
        std::string statement;
        std::string inp;

        cout << "---> ";
        // Read a statement from the keyboard
        // and avoid buffer overflow.

        cin.getline(inbuf, INPUT_MAX_SIZE);

        inp = std::string(inbuf);

        if (inp.empty()) {
            continue;
        }

        // Generate a temporary class with a run method
        stringstream ss;
        ss << SHELL_PREFIX << counter << SHELL_PART_1 << inp << SHELL_PART_2;
        statement = ss.str();

        ++counter;
        runClass = Universe::LoadShellClass(statement);
        // Compile and load the newly generated class
        if (runClass == nullptr) {
            cout << "can't compile statement.";
            continue;
        }

        currentFrame = Interpreter::GetFrame();

        // Go back, so we will evaluate the bootstrap frames halt
        // instruction again
        currentFrame->SetBytecodeIndex(bytecodeIndex);

        // Create and push a new instance of our class on the stack
        currentFrame->Push(Universe::NewInstance(runClass));

        // Push the old value of "it" on the stack
        currentFrame->Push(it);

        // Lookup the run: method
        VMInvokable* initialize = runClass->LookupInvokable(SymbolFor("run:"));

        // Invoke the run method
        initialize->Invoke(currentFrame);

        // Start the Interpreter

        Interpreter::Start(dumpBytecodes > 1);

        // Save the result of the run method
        it = currentFrame->Pop();
    }
}
