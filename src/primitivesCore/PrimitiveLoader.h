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

#include "../misc/defs.h"
#include "Primitives.h"

class PrimitiveContainer;

/// Core class for primitive loading.
// In order to implement new primitive libraries, you can use this class.
//
// Libraries have to take care of initializing any needed data or data
// structures. When using the PrimitiveLoader class that is the the std::map
// primitiveObjects. Initialize it by calling the AddPrimitiveObject method, in
// order to map the name of the smalltalk class to the corresponding
// PrimitiveContainer object.
class PrimitiveLoader {
public:
    PrimitiveLoader();
    virtual ~PrimitiveLoader();
    void AddPrimitiveObject(const std::string& name, PrimitiveContainer* prim);

    static bool SupportsClass(const std::string& name);

    static void InstallPrimitives(const std::string& cname,
                                  VMClass* clazz,
                                  bool classSide);

    static PrimitiveLoader* GetInstance();
    PrimitiveContainer* GetObject(const std::string& name);

private:
    void installPrimitives(const std::string& cname,
                           VMClass* clazz,
                           bool classSide);

    bool supportsClass(const std::string& name);

    std::map<std::string, PrimitiveContainer*> primitiveObjects;

    static PrimitiveLoader loader;
};
