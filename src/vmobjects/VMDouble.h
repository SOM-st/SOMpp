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

#include "AbstractObject.h"

class VMDouble : public AbstractVMObject {
public:
    typedef GCDouble Stored;

    explicit VMDouble(double val) : embeddedDouble(val) {}

    [[nodiscard]] VMDouble* CloneForMovingGC() const override;
    [[nodiscard]] inline double GetEmbeddedDouble() const;
    [[nodiscard]] VMClass* GetClass() const override;
    [[nodiscard]] inline size_t GetObjectSize() const override;

    [[nodiscard]] inline int64_t GetHash() const override {
        // try to avoid a smart cast of the double value.
        // instead, try to get to the bit pattern as a int64_t
        return (*(int64_t*)&embeddedDouble);
    }

    void MarkObjectAsInvalid() override;
    [[nodiscard]] bool IsMarkedInvalid() const override;

    [[nodiscard]] std::string AsDebugString() const override;

private:
    make_testable(public);

    double embeddedDouble;
};

double VMDouble::GetEmbeddedDouble() const {
    return embeddedDouble;
}

size_t VMDouble::GetObjectSize() const {
    return sizeof(VMDouble);
}
