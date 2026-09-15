#pragma once
/**
 * This is a text-book mark-sweep heap.
 *
 * It's a fixed-sized heap that is allocated on creation
 * as a contiguous block of memory.
 *
 * It uses a first-fit free list to allocate objects.
 * The free list is stored in the heap itself.
 * This means, the smallest allocation unit is `sizeof(VMFreeListEntry)`.
 *
 * The free-list uses a first-fit search strategy and is not sorted.
 * If we find a free block that is larger than the requested size,
 * and larger than the requested size + `sizeof(VMFreeListEntry)`, then
 * we split the block and return the part at the end, and adjust the size of the
 * free block.
 */

#include <cstddef>
#include <vector>

#include "../misc/defs.h"
#include "Heap.h"

class VMFreeListEntry;

class MarkSweepHeap : public Heap<MarkSweepHeap> {
    friend class MarkSweepCollector;

public:
    explicit MarkSweepHeap(size_t objectSpaceSize);
    ~MarkSweepHeap();
    void* AllocateObject(size_t size);

    static void findFittingEntry(VMFreeListEntry*& cur, VMFreeListEntry*& prev,
                                 size_t allocSize);

    static constexpr int GC_MARKED = 1;
    static constexpr int GC_UNMARKED = 0;

private:
    void sweep();

    AbstractVMObject* heap;
    AbstractVMObject* heapEnd;
    VMFreeListEntry* freeList;

    uintptr_t liveBytes{0};
    const uintptr_t collectionLimit;
};
