#pragma once

#include "../primitivesCore/PrimitiveContainer.h"
#include "../vmobjects/ObjectFormats.h"

class _Vector : public PrimitiveContainer {
public:
    _Vector();
    void LateInitialize(size_t hash);
};