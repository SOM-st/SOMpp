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

#include "SourcecodeCompiler.h"

#include <fstream>
#include <ios>
#include <sstream>
#include <string>

#include "../misc/defs.h"
#include "../vm/Print.h"
#include "../vm/Universe.h"
#include "../vmobjects/VMClass.h"
#include "../vmobjects/VMSymbol.h"
#include "ClassGenerationContext.h"
#include "Parser.h"

VMClass* SourcecodeCompiler::CompileClass(const std::string& path,
                                          const std::string& file,
                                          VMClass* systemClass) {
    VMClass* result = systemClass;

    std::string fname = path + fileSeparator + file + ".som";

    ifstream fp{};
    fp.open(fname.c_str(), std::ios_base::in);
    if (!fp.is_open()) {
        return nullptr;
    }

    Parser parser(fp, fname);
    result = compile(parser, systemClass);

    VMSymbol const* const cname = result->GetName();
    std::string const cnameC = cname->GetStdString();

    if (file != cnameC) {
        ostringstream Str;
        Str << "Filename: " << file << " does not match class name " << cnameC;

        showCompilationError(file, Str.str().c_str());
        return nullptr;
    }

#ifdef COMPILER_DEBUG
    ErrorPrint("Compilation finished\n");
#endif
    return result;
}

VMClass* SourcecodeCompiler::CompileClassString(const std::string& stream,
                                                VMClass* systemClass) {
    istringstream ss(stream);

    std::string fileName = "repl";
    Parser parser(ss, fileName);

    VMClass* result = compile(parser, systemClass);
    return result;
}

void SourcecodeCompiler::showCompilationError(const std::string& filename,
                                              const char* message) {
    ErrorPrint("Error when compiling " + filename + ":\n" + message + "\n");
}

VMClass* SourcecodeCompiler::compile(Parser& parser, VMClass* systemClass) {
    ClassGenerationContext cgc;

    VMClass* result = systemClass;

    parser.Classdef(cgc);

    if (systemClass == nullptr) {
        result = cgc.Assemble();
    } else {
        cgc.AssembleSystemClass(result);
    }

    return result;
}
