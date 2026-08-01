#include "GenerationalCollector.h"

#include <cassert>
#include <cstddef>
#include <vector>

#include "../misc/debug.h"
#include "../misc/defs.h"  // NOLINT(misc-include-cleaner)
#include "../vm/IsValidObject.h"
#include "../vm/Universe.h"
#include "../vmobjects/IntegerBox.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMFrame.h"
#include "../vmobjects/VMObjectBase.h"
#include "GarbageCollector.h"

GenerationalCollector::GenerationalCollector(GenerationalHeap* heap)
    : GarbageCollector(heap) {}

static gc_oop_t mark_object(gc_oop_t oop) {
    // don't process tagged objects
    if (IS_TAGGED(oop)) {
        return oop;
    }

    AbstractVMObject* obj = AS_OBJ(oop);
    assert(IsValidObject(obj));

    if ((obj->GetGCField() & MASK_OBJECT_IS_MARKED) != 0) {
        return oop;
    }

    obj->SetGCField(MASK_OBJECT_IS_OLD | MASK_OBJECT_IS_MARKED);
    obj->WalkObjects(&mark_object);

    return oop;
}

static gc_oop_t copy_if_necessary(gc_oop_t oop) {
    // don't process tagged objects
    if (IS_TAGGED(oop)) {
        return oop;
    }

    AbstractVMObject* obj = AS_OBJ(oop);
    assert(IsValidObject(obj));

    size_t const gcField = obj->GetGCField();

    // if this is an old object already, we don't have to copy
    if ((gcField & MASK_OBJECT_IS_OLD) != 0) {
        return oop;
    }

    // GCField is abused as forwarding pointer here
    // if someone has moved before, return the moved object
    if (gcField != 0) {
        return (gc_oop_t)gcField;
    }

    // we have to clone ourselves
    AbstractVMObject* newObj = obj->CloneForMovingGC();

    if (DEBUG) {
        obj->MarkObjectAsInvalid();
    }

    assert((((size_t)newObj) & MASK_OBJECT_IS_MARKED) == 0);
    assert(obj->GetObjectSize() == newObj->GetObjectSize());

    obj->SetGCField((size_t)newObj);
    newObj->SetGCField(MASK_OBJECT_IS_OLD);

    // walk recursively
    newObj->WalkObjects(copy_if_necessary);

    return tmp_ptr(newObj);
}

void GenerationalCollector::MinorCollection() {
    DebugLog("GenGC MinorCollection\n");

    // walk all globals of universe, and implicily the interpreter
    Universe::WalkGlobals(&copy_if_necessary);

    // and also all objects that have been detected by the write barriers
    for (size_t const& oldObj : heap->oldObjsWithRefToYoungObjs) {
        // content of oldObjsWithRefToYoungObjs is not altered while iteration,
        // because copy_if_necessary returns old objs only -> ignored by
        // write_barrier
        auto* obj = (AbstractVMObject*)oldObj;
        obj->SetGCField(MASK_OBJECT_IS_OLD);
        obj->WalkObjects(&copy_if_necessary);
    }
    heap->oldObjsWithRefToYoungObjs.clear();
    heap->nextFreePosition = heap->nursery;
}

void GenerationalCollector::MajorCollection() {
    DebugLog("GenGC MajorCollection\n");

    // first we have to mark all objects (globals and current frame recursively)
    Universe::WalkGlobals(&mark_object);

    // now that all objects are marked we can safely delete all allocated
    // objects that are not marked
    vector<AbstractVMObject*> survivors;
    for (auto* obj : heap->allocatedObjects) {
        assert(IsValidObject(obj));

        if ((obj->GetGCField() & MASK_OBJECT_IS_MARKED) != 0) {
            survivors.push_back(obj);
            obj->SetGCField(MASK_OBJECT_IS_OLD);
        } else {
            heap->FreeObject(obj);
        }
    }
    heap->allocatedObjects.swap(survivors);
}

void GenerationalCollector::Collect() {
    DebugLog("GenGC Collect\n");
    Timer::GCTimer.Resume();
    // reset collection trigger
    heap->resetGCTrigger();

    MinorCollection();
    if (heap->matureObjectsSize > majorCollectionThreshold) {
        MajorCollection();
        majorCollectionThreshold = 2 * heap->matureObjectsSize;
    }
    Timer::GCTimer.Halt();
}
