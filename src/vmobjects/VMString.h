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

class VMString : public AbstractVMObject {
public:
    typedef GCString Stored;

    VMString(const size_t length, const char* str)
        : length(length),
          // set the chars-pointer to point at the position of the first
          // character
          chars((char*)&chars + sizeof(char*)) {
        for (size_t i = 0; i < length; i++) {
            chars[i] = str[i];
        }
    }

    [[nodiscard]] int64_t GetHash() const override {
        uint64_t hash = 5381U;

        for (size_t i = 0; i < length; i++) {
            hash = ((hash << 5U) + hash) + chars[i];
        }

        return (int64_t)hash;
    }

    /**
     * There's no guarantee that this is null-terminated.
     * Indeed it's highly unlikely to be.
     * Need to always use with GetStringLength().
     */
    [[nodiscard]] inline char* GetRawChars() const { return chars; }

    [[nodiscard]] std::string GetStdString() const;

    [[nodiscard]] inline size_t GetStringLength() const { return length; }

    [[nodiscard]] VMString* CloneForMovingGC() const override;
    [[nodiscard]] VMClass* GetClass() const override;
    [[nodiscard]] size_t GetObjectSize() const override;

    inline void WalkObjects(walk_heap_fn /*unused*/) override {
        // nothing to do
    }

    void MarkObjectAsInvalid() override {
        for (size_t i = 0; i < length; i++) {
            chars[i] = 'z';
        }
        chars = (char*)INVALID_GC_POINTER;
    }

    [[nodiscard]] bool IsMarkedInvalid() const override {
        return chars == (char*)INVALID_GC_POINTER;
    }

    [[nodiscard]] std::string AsDebugString() const override;

    make_testable(public);

    // this could be replaced by the CHARS macro in VMString.cpp
    // in order to decrease the object size
    const size_t length;
    char* chars;

protected:
    VMString(char* const adaptedCharsPointer, const size_t length)
        :  // set the chars-pointer to point at the position of the first
           // character as determined in the VMSymbol constructor
          length(length),
          chars(adaptedCharsPointer) {};  // constructor to use by VMSymbol
};
