#pragma once

#include "../primitivesCore/PrimitiveContainer.h"
#include "../vmobjects/ObjectFormats.h"

// TODO(RiceDope): remove test fails due to subclassing of the Vector.som class.
// Ensure that subclassing will also use a VMVector object for errors.

class _Vector : public PrimitiveContainer {
public:
    _Vector();
};