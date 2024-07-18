#pragma once

#include "../misc/defs.h"
#include "../vmobjects/ObjectFormats.h"

bool IsValidObject(vm_oop_t obj);

void set_vt_to_null();

void obtain_vtables_of_known_classes(VMSymbol* className);

