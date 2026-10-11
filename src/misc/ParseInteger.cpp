#include "ParseInteger.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "../vm/Universe.h"  // NOLINT(misc-include-cleaner)
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMBigInteger.h"  // NOLINT(misc-include-cleaner)

vm_oop_t ParseInteger(const char* str, bool negateValue) {
    errno = 0;

    char* pEnd{};

    const int64_t i = std::strtoll(str, &pEnd, 10);

    if (str == pEnd) {
        // did not parse anything
        return NEW_INT(0);
    }

    // the digits don't fit an int64_t, but a negated value still might, so
    // NewInt decides
    const bool rangeError = errno == ERANGE;
    if (rangeError) {
        return Universe::NewInt(negateValue ? -InfInt(str) : InfInt(str));
    }

    // the normal case
    if (negateValue) {
        return NEW_INT(-i);
    }
    return NEW_INT(i);
}

vm_oop_t ParseInteger(std::string& str, bool negateValue) {
    return ParseInteger(str.c_str(), negateValue);
}
