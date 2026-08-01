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

#include "Integer.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <string>

#include "../lib/InfInt.h"
#include "../misc/ParseInteger.h"
#include "../misc/defs.h"
#include "../vm/Globals.h"
#include "../vm/Print.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMBigInteger.h"
#include "../vmobjects/VMDouble.h"  // NOLINT(misc-include-cleaner)
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMString.h"

//
// arithmetic operations
//

#define doSmallIntWithDoubleOp(leftInt, rightObj, op)    \
    {                                                    \
        double const leftDbl = (double)(leftInt);        \
        double const rightDbl = AS_DOUBLE(rightObj);     \
        return Universe::NewDouble(leftDbl op rightDbl); \
    }

static vm_oop_t intPlus(vm_oop_t leftObj, vm_oop_t rightObj) {
    assert(CLASS_OF(leftObj) == load_ptr(integerClass) &&
           "The receiver should always be an int");
    if (IS_SMALL_INT(leftObj)) {
        int64_t const left = SMALL_INT_VAL(leftObj);

        if (IS_SMALL_INT(rightObj)) {
            int64_t const right = SMALL_INT_VAL(rightObj);
            int64_t result = 0;
            if (unlikely(__builtin_add_overflow(left, right, &result))) {
                InfInt const l(left);
                InfInt const r(right);
                return Universe::NewInt(l + r);
            }
            return NEW_INT(result);
        }

        if (IS_DOUBLE(rightObj)) {
            doSmallIntWithDoubleOp(left, rightObj, +);
        }

        assert(IS_BIG_INT(rightObj) && "assume we're having a big int now");
        return AS_BIG_INT(rightObj)->Add(left);
    }

    assert(IS_BIG_INT(leftObj) && "assume rcvr is a big int now");
    return AS_BIG_INT(leftObj)->Add(rightObj);
}

static vm_oop_t intBitwiseXor(vm_oop_t leftObj, vm_oop_t rightObj) {
    if (IS_SMALL_INT(leftObj) && IS_SMALL_INT(rightObj)) {
        int64_t const left = SMALL_INT_VAL(leftObj);

        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        int64_t const result = left ^ SMALL_INT_VAL(rightObj);
        return NEW_INT(result);
    }

    ErrorExit("#xor: not supported on non-small-int arguments");
}

static vm_oop_t intLeftShift(vm_oop_t leftObj, vm_oop_t rightObj) {
    if (IS_SMALL_INT(leftObj) && IS_SMALL_INT(rightObj)) {
        int64_t const left = SMALL_INT_VAL(leftObj);
        int64_t const right = SMALL_INT_VAL(rightObj);

        // NOLINTNEXTLINE(misc-include-cleaner)
        auto const numberOfLeadingZeros = __builtin_clzll((uint64_t)left);

        if (64 - numberOfLeadingZeros + right > 63) {
            return Universe::NewInt(InfInt(left) << right);
        }

        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        int64_t const result = left << right;
        return NEW_INT(result);
    }

    ErrorExit("#<< not supported on non-small-int arguments");
}

static vm_oop_t intUnsignedRightShift(vm_oop_t leftObj, vm_oop_t rightObj) {
    if (IS_SMALL_INT(leftObj) && IS_SMALL_INT(rightObj)) {
        uint64_t const left = SMALL_INT_VAL(leftObj);
        uint64_t const right = SMALL_INT_VAL(rightObj);

        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        auto const result = static_cast<int64_t>(left >> right);
        return NEW_INT(result);
    }

    ErrorExit("#<< not supported on non-small-int arguments");
}

static vm_oop_t intMinus(vm_oop_t leftObj, vm_oop_t rightObj) {
    assert(CLASS_OF(leftObj) == load_ptr(integerClass) &&
           "The receiver should always be an int");
    if (IS_SMALL_INT(leftObj)) {
        int64_t const left = SMALL_INT_VAL(leftObj);

        if (IS_SMALL_INT(rightObj)) {
            int64_t const right = SMALL_INT_VAL(rightObj);
            int64_t result = 0;
            if (unlikely(__builtin_sub_overflow(left, right, &result))) {
                InfInt const l(left);
                InfInt const r(right);
                return Universe::NewInt(l - r);
            }
            return NEW_INT(result);
        }

        if (IS_DOUBLE(rightObj)) {
            doSmallIntWithDoubleOp(left, rightObj, -);
        }

        assert(IS_BIG_INT(rightObj) && "assume we're having a big int now");
        return AS_BIG_INT(rightObj)->SubtractFrom(left);
    }

    assert(IS_BIG_INT(leftObj) && "assume rcvr is a big int now");
    return AS_BIG_INT(leftObj)->Subtract(rightObj);
}

static vm_oop_t intStar(vm_oop_t leftObj, vm_oop_t rightObj) {
    assert(CLASS_OF(leftObj) == load_ptr(integerClass) &&
           "The receiver should always be an int");
    if (IS_SMALL_INT(leftObj)) {
        int64_t const left = SMALL_INT_VAL(leftObj);

        if (IS_SMALL_INT(rightObj)) {
            int64_t const right = SMALL_INT_VAL(rightObj);
            int64_t result = 0;
            if (unlikely(__builtin_mul_overflow(left, right, &result))) {
                InfInt const l(left);
                InfInt const r(right);
                return Universe::NewInt(l * r);
            }
            return NEW_INT(result);
        }

        if (IS_DOUBLE(rightObj)) {
            doSmallIntWithDoubleOp(left, rightObj, *);
        }

        assert(IS_BIG_INT(rightObj) && "assume we're having a big int now");
        return AS_BIG_INT(rightObj)->Multiply(left);
    }

    assert(IS_BIG_INT(leftObj) && "assume rcvr is a big int now");
    return AS_BIG_INT(leftObj)->Multiply(rightObj);
}

static vm_oop_t intSlashslash(vm_oop_t leftObj, vm_oop_t rightObj) {
    double left = NAN;
    if (IS_SMALL_INT(leftObj)) {
        left = (double)SMALL_INT_VAL(leftObj);
    } else {
        assert(IS_BIG_INT(leftObj) && "should be a big integer now");
        left = AS_BIG_INT(leftObj)->embeddedInteger.toDouble();
    }

    double right = NAN;
    if (IS_SMALL_INT(rightObj)) {
        right = (double)SMALL_INT_VAL(rightObj);
    } else if (IS_DOUBLE(rightObj)) {
        right = AS_DOUBLE(rightObj);
    } else {
        assert(IS_BIG_INT(leftObj) && "should be a big integer now");
        right = AS_BIG_INT(leftObj)->embeddedInteger.toDouble();
    }

    double const result = left / right;
    return Universe::NewDouble(result);
}

static vm_oop_t intSlash(vm_oop_t leftObj, vm_oop_t rightObj) {
    if (IS_SMALL_INT(leftObj)) {
        int64_t const left = SMALL_INT_VAL(leftObj);

        if (IS_SMALL_INT(rightObj)) {
            int64_t const right = SMALL_INT_VAL(rightObj);
            int64_t const result = left / right;
            return NEW_INT(result);
        }

        if (IS_DOUBLE(rightObj)) {
            double const right = AS_DOUBLE(rightObj);
            auto const result = (int64_t)(static_cast<double>(left) / right);
            return NEW_INT(result);
        }

        assert(IS_BIG_INT(rightObj) && "expected to be a big int now");
        return AS_BIG_INT(rightObj)->DivisionFrom(left);
    }

    assert(IS_BIG_INT(leftObj));
    return AS_BIG_INT(leftObj)->DivideBy(rightObj);
}

static vm_oop_t intPercent(vm_oop_t leftObj, vm_oop_t rightObj) {
    if (IS_SMALL_INT(leftObj)) {
        int64_t const left = SMALL_INT_VAL(leftObj);

        if (IS_SMALL_INT(rightObj)) {
            int64_t const right = SMALL_INT_VAL(rightObj);
            int64_t result = left % right;

            if ((result != 0) && ((result < 0) != (right < 0))) {
                result += right;
            }
            return NEW_INT(result);
        }

        if (IS_DOUBLE(rightObj)) {
            auto const right = (int64_t)AS_DOUBLE(rightObj);
            int64_t result = left % right;

            if ((result != 0) && ((result < 0) != (right < 0))) {
                result += right;
            }
            return NEW_INT(result);
        }

        assert(IS_BIG_INT(rightObj) && "expected to be a big int now");
        return AS_BIG_INT(rightObj)->ModuloFrom(left);
    }

    assert(IS_BIG_INT(leftObj));
    return AS_BIG_INT(leftObj)->Modulo(rightObj);
}

static vm_oop_t intRem(vm_oop_t leftObj, vm_oop_t rightObj) {
    if (IS_SMALL_INT(leftObj)) {
        int64_t const left = SMALL_INT_VAL(leftObj);

        if (IS_SMALL_INT(rightObj)) {
            int64_t const right = SMALL_INT_VAL(rightObj);
            int64_t const result = left - ((left / right) * right);

            return NEW_INT(result);
        }

        ErrorExit("not yet implemented #rem: small-int and something else");
    }

    ErrorExit("not yet implemented #rem: big int and something else");
}

static vm_oop_t intAnd(vm_oop_t leftObj, vm_oop_t rightObj) {
    if (IS_SMALL_INT(leftObj) && IS_SMALL_INT(rightObj)) {
        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        int64_t const result = SMALL_INT_VAL(leftObj) & SMALL_INT_VAL(rightObj);
        return NEW_INT(result);
    }

    if (IS_BIG_INT(leftObj) && IS_SMALL_INT(rightObj)) {
        VMBigInteger const* const left = AS_BIG_INT(leftObj);
        int64_t const l = left->embeddedInteger.truncateToInt64();
        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        return NEW_INT(l & SMALL_INT_VAL(rightObj));
    }

    ErrorExit("#& not supported on non-small-int arguments");
}

static vm_oop_t intEqual(vm_oop_t leftObj, vm_oop_t rightObj) {
    if (IS_SMALL_INT(leftObj)) {
        int64_t const left = SMALL_INT_VAL(leftObj);

        if (IS_SMALL_INT(rightObj)) {
            int64_t const right = SMALL_INT_VAL(rightObj);
            return left == right ? load_ptr(trueObject) : load_ptr(falseObject);
        }

        if (IS_DOUBLE(rightObj)) {
            auto const leftDouble = (double)left;
            double const right = AS_DOUBLE(rightObj);
            return leftDouble == right ? load_ptr(trueObject)
                                       : load_ptr(falseObject);
        }

        return load_ptr(falseObject);
    }

    assert(IS_BIG_INT(leftObj));
    if (!IS_BIG_INT(rightObj)) {
        return load_ptr(falseObject);
    }
    return AS_BIG_INT(leftObj)->IsEqual(AS_BIG_INT(rightObj));
}

static vm_oop_t intEqualEqual(vm_oop_t leftObj, vm_oop_t rightObj) {
    if (IS_SMALL_INT(leftObj)) {
        if (IS_SMALL_INT(rightObj)) {
            return SMALL_INT_VAL(leftObj) == SMALL_INT_VAL(rightObj)
                       ? load_ptr(trueObject)
                       : load_ptr(falseObject);
        }
        return load_ptr(falseObject);
    }

    assert(IS_BIG_INT(leftObj));
    if (!IS_BIG_INT(rightObj)) {
        return load_ptr(falseObject);
    }
    return AS_BIG_INT(leftObj)->IsEqual(AS_BIG_INT(rightObj));
}

inline static bool lowerThan(vm_oop_t leftObj, vm_oop_t rightObj) {
    if (IS_SMALL_INT(leftObj)) {
        int64_t const left = SMALL_INT_VAL(leftObj);
        if (IS_SMALL_INT(rightObj)) {
            return left < SMALL_INT_VAL(rightObj);
        }

        if (IS_DOUBLE(rightObj)) {
            double const right = AS_DOUBLE(rightObj);
            return static_cast<double>(left) < right;
        }

        assert(IS_BIG_INT(rightObj));
        return AS_BIG_INT(rightObj)->embeddedInteger >= InfInt(left);
    }

    assert(IS_BIG_INT(leftObj) && "assume big int");
    VMBigInteger const* const left = AS_BIG_INT(leftObj);

    if (IS_SMALL_INT(rightObj)) {
        return left->embeddedInteger < InfInt(SMALL_INT_VAL(rightObj));
    }

    if (IS_DOUBLE(rightObj)) {
        ErrorExit("#< on big int not supported with double");
        //        return (left->embeddedInteger < AS_DOUBLE(rightObj)) ?
        //        load_ptr(trueObject) : load_ptr(falseObject);
    }

    assert(IS_BIG_INT(rightObj) && "assume big int");
    return left->embeddedInteger < AS_BIG_INT(rightObj)->embeddedInteger;
}

static vm_oop_t intLowerthan(vm_oop_t leftObj, vm_oop_t rightObj) {
    return lowerThan(leftObj, rightObj) ? load_ptr(trueObject)
                                        : load_ptr(falseObject);
}

static vm_oop_t intLowerThanEqual(vm_oop_t leftObj, vm_oop_t rightObj) {
    if (IS_SMALL_INT(leftObj)) {
        int64_t const left = SMALL_INT_VAL(leftObj);

        if (IS_SMALL_INT(rightObj)) {
            return left <= SMALL_INT_VAL(rightObj) ? load_ptr(trueObject)
                                                   : load_ptr(falseObject);
        }

        if (IS_DOUBLE(rightObj)) {
            double const right = AS_DOUBLE(rightObj);
            return static_cast<double>(left) <= right ? load_ptr(trueObject)
                                                      : load_ptr(falseObject);
        }

        assert(IS_BIG_INT(rightObj));
        return AS_BIG_INT(rightObj)->embeddedInteger > InfInt(left)
                   ? load_ptr(trueObject)
                   : load_ptr(falseObject);
    }

    assert(IS_BIG_INT(leftObj) && "assume big int");
    VMBigInteger const* const left = AS_BIG_INT(leftObj);

    if (IS_SMALL_INT(rightObj)) {
        return (left->embeddedInteger <= InfInt(SMALL_INT_VAL(rightObj)))
                   ? load_ptr(trueObject)
                   : load_ptr(falseObject);
    }

    if (IS_DOUBLE(rightObj)) {
        ErrorExit("#< on big int not supported with double");
        //        return (left->embeddedInteger <= AS_DOUBLE(rightObj)) ?
        //        load_ptr(trueObject) : load_ptr(falseObject);
    }

    assert(IS_BIG_INT(rightObj) && "assume big int");
    return (left->embeddedInteger <= AS_BIG_INT(rightObj)->embeddedInteger)
               ? load_ptr(trueObject)
               : load_ptr(falseObject);
}

static vm_oop_t intGreaterThan(vm_oop_t leftObj, vm_oop_t rightObj) {
    if (IS_SMALL_INT(leftObj)) {
        int64_t const left = SMALL_INT_VAL(leftObj);

        if (IS_SMALL_INT(rightObj)) {
            return left > SMALL_INT_VAL(rightObj) ? load_ptr(trueObject)
                                                  : load_ptr(falseObject);
        }

        if (IS_DOUBLE(rightObj)) {
            double const right = AS_DOUBLE(rightObj);
            return static_cast<double>(left) > right ? load_ptr(trueObject)
                                                     : load_ptr(falseObject);
        }

        assert(IS_BIG_INT(rightObj));
        return AS_BIG_INT(rightObj)->embeddedInteger < InfInt(left)
                   ? load_ptr(trueObject)
                   : load_ptr(falseObject);
    }

    assert(IS_BIG_INT(leftObj) && "assume big int");
    VMBigInteger const* const left = AS_BIG_INT(leftObj);

    if (IS_SMALL_INT(rightObj)) {
        return (left->embeddedInteger > InfInt(SMALL_INT_VAL(rightObj)))
                   ? load_ptr(trueObject)
                   : load_ptr(falseObject);
    }

    if (IS_DOUBLE(rightObj)) {
        ErrorExit("#< on big int not supported with double");
        //        return (left->embeddedInteger > AS_DOUBLE(rightObj)) ?
        //        load_ptr(trueObject) : load_ptr(falseObject);
    }

    assert(IS_BIG_INT(rightObj) && "assume big int");
    return (left->embeddedInteger > AS_BIG_INT(rightObj)->embeddedInteger)
               ? load_ptr(trueObject)
               : load_ptr(falseObject);
}

static vm_oop_t intGreaterThanEqual(vm_oop_t leftObj, vm_oop_t rightObj) {
    if (IS_SMALL_INT(leftObj)) {
        int64_t const left = SMALL_INT_VAL(leftObj);

        if (IS_SMALL_INT(rightObj)) {
            return left >= SMALL_INT_VAL(rightObj) ? load_ptr(trueObject)
                                                   : load_ptr(falseObject);
        }

        if (IS_DOUBLE(rightObj)) {
            double const right = AS_DOUBLE(rightObj);
            return static_cast<double>(left) >= right ? load_ptr(trueObject)
                                                      : load_ptr(falseObject);
        }

        assert(IS_BIG_INT(rightObj));
        return AS_BIG_INT(rightObj)->embeddedInteger < InfInt(left)
                   ? load_ptr(trueObject)
                   : load_ptr(falseObject);
    }

    assert(IS_BIG_INT(leftObj) && "assume big int");
    VMBigInteger const* const left = AS_BIG_INT(leftObj);

    if (IS_SMALL_INT(rightObj)) {
        return (left->embeddedInteger >= InfInt(SMALL_INT_VAL(rightObj)))
                   ? load_ptr(trueObject)
                   : load_ptr(falseObject);
    }

    if (IS_DOUBLE(rightObj)) {
        ErrorExit("#< on big int not supported with double");
        //        return (left->embeddedInteger >= AS_DOUBLE(rightObj)) ?
        //        load_ptr(trueObject) : load_ptr(falseObject);
    }

    assert(IS_BIG_INT(rightObj) && "assume big int");
    return (left->embeddedInteger >= AS_BIG_INT(rightObj)->embeddedInteger)
               ? load_ptr(trueObject)
               : load_ptr(falseObject);
}

static vm_oop_t intAsString(vm_oop_t self) {
    if (IS_SMALL_INT(self)) {
        int64_t const integer = SMALL_INT_VAL(self);
        ostringstream Str;
        Str << integer;
        return Universe::NewString(Str.str());
    }

    assert(IS_BIG_INT(self) && "assume big int");
    return Universe::NewString(AS_BIG_INT(self)->embeddedInteger.toString());
}

static vm_oop_t intAsDouble(vm_oop_t self) {
    double value = NAN;
    if (IS_SMALL_INT(self)) {
        value = (double)SMALL_INT_VAL(self);
    } else {
        assert(IS_BIG_INT(self) && "assume big int");
        value = AS_BIG_INT(self)->embeddedInteger.toDouble();
    }
    return Universe::NewDouble(value);
}

static vm_oop_t intAs32BitSigned(vm_oop_t self) {
    int32_t value = 0;
    if (IS_SMALL_INT(self)) {
        value = (int32_t)SMALL_INT_VAL(self);
    } else {
        assert(IS_BIG_INT(self) && "assume big int");
        value = AS_BIG_INT(self)->embeddedInteger.truncateToInt();
    }
    return NEW_INT((int64_t)value);
}

static vm_oop_t intAs32BitUnsigned(vm_oop_t self) {
    uint32_t value = 0;
    if (IS_SMALL_INT(self)) {
        value = (uint32_t)SMALL_INT_VAL(self);
    } else {
        assert(IS_BIG_INT(self) && "assume big int");
        value = (uint32_t)AS_BIG_INT(self)->embeddedInteger.truncateToInt();
    }
    return NEW_INT((int64_t)value);
}

static vm_oop_t intSqrt(vm_oop_t self) {
    double const result = sqrt((double)SMALL_INT_VAL(self));

    if (result == rint(result)) {
        return NEW_INT((int64_t)result);
    }
    return Universe::NewDouble(result);
}

static vm_oop_t intAtRandom(vm_oop_t self) {
    int64_t const result =
        SMALL_INT_VAL(self) *
        rand();  // NOLINT(clang-analyzer-security.insecureAPI.rand,misc-predictable-rand)
    return NEW_INT(result);
}

static vm_oop_t intAbs(vm_oop_t self) {
    if (IS_SMALL_INT(self)) {
        int64_t const result = SMALL_INT_VAL(self);
        if (result < 0) {
            return NEW_INT(-result);
        }
        return self;
    }

    assert(IS_BIG_INT(self) && "assume big int");
    VMBigInteger* s = AS_BIG_INT(self);
    return s->Negate();
}

static vm_oop_t intMin(vm_oop_t self, vm_oop_t arg) {
    return lowerThan(self, arg) ? self : arg;
}

static vm_oop_t intMax(vm_oop_t self, vm_oop_t arg) {
    return lowerThan(self, arg) ? arg : self;
}

static vm_oop_t intFromString(vm_oop_t /*unused*/, vm_oop_t right) {
    auto* self = (VMString*)right;
    std::string str = self->GetStdString();

    return ParseInteger(str, false);
}

static vm_oop_t intUnequal(vm_oop_t leftObj, vm_oop_t rightObj) {
    if (IS_SMALL_INT(leftObj)) {
        int64_t const left = SMALL_INT_VAL(leftObj);

        if (IS_SMALL_INT(rightObj)) {
            return left != SMALL_INT_VAL(rightObj) ? load_ptr(trueObject)
                                                   : load_ptr(falseObject);
        }

        if (IS_DOUBLE(rightObj)) {
            double const right = AS_DOUBLE(rightObj);
            return static_cast<double>(left) != right ? load_ptr(trueObject)
                                                      : load_ptr(falseObject);
        }

        if (IS_BIG_INT(rightObj)) {
            return AS_BIG_INT(rightObj)->embeddedInteger != InfInt(left)
                       ? load_ptr(trueObject)
                       : load_ptr(falseObject);
        }

        return load_ptr(trueObject);
    }

    assert(IS_BIG_INT(leftObj) && "assume big int");
    VMBigInteger const* const left = AS_BIG_INT(leftObj);

    if (IS_SMALL_INT(rightObj)) {
        return (left->embeddedInteger != InfInt(SMALL_INT_VAL(rightObj)))
                   ? load_ptr(trueObject)
                   : load_ptr(falseObject);
    }

    if (IS_DOUBLE(rightObj)) {
        ErrorExit("#< on big int not supported with double");
        //        return (left->embeddedInteger >= AS_DOUBLE(rightObj)) ?
        //        load_ptr(trueObject) : load_ptr(falseObject);
    }

    if (IS_BIG_INT(rightObj)) {
        return (left->embeddedInteger != AS_BIG_INT(rightObj)->embeddedInteger)
                   ? load_ptr(trueObject)
                   : load_ptr(falseObject);
    }
    return load_ptr(trueObject);
}

static vm_oop_t intRange(vm_oop_t leftObj, vm_oop_t rightObj) {
    int64_t const left = SMALL_INT_VAL(leftObj);
    int64_t const right = SMALL_INT_VAL(rightObj);

    int64_t const numInteger = right - left + 1;
    VMArray* arr = Universe::NewArray(numInteger);

    size_t index = 0;
    for (int64_t i = left; i <= right; i += 1, index += 1) {
        arr->SetIndexableField(index, NEW_INT(i));
    }

    return arr;
}

_Integer::_Integer() {
    srand((unsigned)time(nullptr));  // NOLINT(bugprone-random-generator-seed)

    Add("+", &intPlus, false);
    Add("-", &intMinus, false);
    Add("*", &intStar, false);
    Add("rem:", &intRem, false);

    Add("bitXor:", &intBitwiseXor, false);
    Add("<<", &intLeftShift, false);
    Add(">>>", &intUnsignedRightShift, false);
    Add("/", &intSlash, false);
    Add("//", &intSlashslash, false);
    Add("%", &intPercent, false);
    Add("&", &intAnd, false);
    Add("=", &intEqual, false);
    Add("==", &intEqualEqual, false);
    Add("<", &intLowerthan, false);
    Add("asString", &intAsString, false);
    Add("asDouble", &intAsDouble, false);
    Add("as32BitSignedValue", &intAs32BitSigned, false);
    Add("as32BitUnsignedValue", &intAs32BitUnsigned, false);
    Add("sqrt", &intSqrt, false);
    Add("atRandom", &intAtRandom, false);
    Add("fromString:", &intFromString, true);

    Add("<=", &intLowerThanEqual, false);
    Add(">", &intGreaterThan, false);
    Add(">=", &intGreaterThanEqual, false);
    Add("<>", &intUnequal, false);
    Add("~=", &intUnequal, false);

    Add("abs", &intAbs, false);
    Add("min:", &intMin, false);
    Add("max:", &intMax, false);

    Add("to:", &intRange, false);
}
