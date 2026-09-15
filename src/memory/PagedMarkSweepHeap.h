#pragma once
/*
 * A mark-and-sweep garbage collector.
 *
 * The heap is divided into pages, and a page represents a single size class.
 * There are a lot of size classes, one for every 8-byte size.
 * Thus, sizes are precise and have no internal fragmentation beyond alignment.
 * The maximum size class is MAX_SMALL_OBJECT_SIZE (2048 bytes).
 * Larger objects are allocated directly with malloc.
 *
 * Marking happens after a GC was triggered.
 *
 * Sweeping is lazy, except for large objects.
 *
 * A collection itself only marks. Dead objects are reclaimed as part of
 * allocation when there's no free space in the size class.
 */

#include <cstddef>
#include <vector>

#include "../misc/defs.h"
#include "Heap.h"

class PagedMarkSweepHeap : public Heap<PagedMarkSweepHeap> {
    friend class PagedMarkSweepCollector;
    struct FreeListEntry {
        FreeListEntry* next;
    };
    // NOLINTNEXTLINE(altera-struct-pack-align): FPGA-specific, not relevant
    struct Page {
        char* memory;
        size_t sweptLastAtEpoch;
    };

public:
    explicit PagedMarkSweepHeap(size_t objectSpaceSize);
    ~PagedMarkSweepHeap();
    void* AllocateObject(size_t size);

private:
    static size_t sizeClassIndex(size_t size);
    void carveNewPage(size_t classIndex);
    bool sweepPageAt(size_t classIndex, size_t pageIndex);
    bool sweepNextPage(size_t classIndex);
    void* allocateLargeObject(size_t size);
    void accountAllocation(size_t bytes);

    std::vector<FreeListEntry*> freeLists;        // per size class
    std::vector<std::vector<Page*>> classPages;   // per size class
    std::vector<size_t> sweepCursor;              // per size class
    std::vector<AbstractVMObject*> largeObjects;  // not page-allocated

    size_t epoch{0};  // current live mark; bumped each collection
    size_t spcAlloc{0};
    size_t collectionLimit;
    // floor for collectionLimit (~the configured heap size): collect only once
    // about a heap's worth has been allocated, like the copying collector,
    // rather than every ~live-set bytes.
    size_t minCollectionLimit;
};
