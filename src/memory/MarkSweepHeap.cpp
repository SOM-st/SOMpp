#include "MarkSweepHeap.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include "../memory/Heap.h"
#include "../vm/IsValidObject.h"
#include "../vm/Print.h"
#include "../vmobjects/AbstractObject.h"
#include "MarkSweepCollector.h"

MarkSweepHeap::MarkSweepHeap(size_t objectSpaceSize)
    : Heap<MarkSweepHeap>(new MarkSweepCollector(this)),
      heap(static_cast<AbstractVMObject*>(malloc(objectSpaceSize))),
      // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
      heapEnd(reinterpret_cast<AbstractVMObject*>(
          // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
          reinterpret_cast<char*>(heap) + objectSpaceSize)),
      freeList(new (heap) VMFreeListEntry(objectSpaceSize, nullptr)),

      collectionLimit((uintptr_t)((double)objectSpaceSize * 0.9)) {
    if (heap == nullptr) {
        ErrorPrint("\nFailed to allocate a " + to_string(objectSpaceSize) +
                   " Byte heap.\n");
        Quit(-1);
    }

    freeList->size = objectSpaceSize;
    freeList->next = nullptr;

    VMFreeListEntry::AssertVMObjectStructure();
}

MarkSweepHeap::~MarkSweepHeap() {
    free(heap);
}

void* MarkSweepHeap::AllocateObject(size_t size) {
    // first fit allocation, searching the free list
    VMFreeListEntry* prev = nullptr;
    VMFreeListEntry* cur = freeList;

    size_t const allocSize = MarkSweepCollector::GetAllocationSize(size);

    findFittingEntry(cur, prev, allocSize);
    if (cur == nullptr) {
        ErrorPrint("\nFailed to allocate " + to_string(size) +
                   " bytes in heap.\n");
        Quit(-1);
    }

    // acquire the memory for the `newObject` by
    // either splitting the free block or removing it from the free list
    void* newObject = nullptr;
    if (cur->LargeEnoughForSplitting(allocSize)) {
        size_t const remainingSize = cur->size - allocSize;
        cur->size = remainingSize;
        newObject =
            // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
            static_cast<void*>(reinterpret_cast<char*>(cur) + remainingSize);
    } else {
        newObject = static_cast<void*>(cur);
        if (prev == nullptr) {
            freeList = cur->next;
        } else {
            prev->next = cur->next;
        }
    }

    liveBytes += allocSize;
    if (liveBytes >= collectionLimit || gcStressMode) {
        requestGC();
    }

    return newObject;
}

void MarkSweepHeap::findFittingEntry(VMFreeListEntry*& cur,
                                     VMFreeListEntry*& prev,
                                     size_t allocSize) {
    while (cur != nullptr) {
        // we can take entries that fit exactly and entries that we can split
        // we can't use entries that have just a little more memory,
        // because then we can't represent the info that there's a bit that's
        // empty after the object.
        if (cur->size == allocSize || cur->LargeEnoughForSplitting(allocSize)) {
            break;
        }
        prev = cur;
        cur = cur->next;
    }
}

void MarkSweepHeap::sweep() {
    freeList = nullptr;  // we are rebuilding the free list from scratch
    AbstractVMObject* cur = heap;

    while (cur < heapEnd) {
        size_t const allocSize =
            MarkSweepCollector::GetAllocationSize(cur->GetObjectSize());
        if (cur->GetGCField() == GC_UNMARKED) {
            // an unreachable object, add it to `freeList`
            if (freeList == nullptr) {
                // the freeList is empty
                freeList = new (cur) VMFreeListEntry(allocSize, nullptr);
            } else if (freeList->IsEntryDirectlyFollowedBy(cur)) {
                // previous free block is directly before this one: merge them
                freeList->size += allocSize;
            } else {
                // a new free block, add it to the front of the free list
                auto* newEntry = new (cur) VMFreeListEntry(allocSize, freeList);
                freeList = newEntry;
            }
        } else {
            // a reachable object, reset its mark for the next collection
            assert(IsValidObject(cur));
            cur->SetGCField(GC_UNMARKED);
        }

        // go to next object in the heap
        // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
        cur = reinterpret_cast<AbstractVMObject*>(reinterpret_cast<char*>(cur) +
                                                  allocSize);
    }
}
