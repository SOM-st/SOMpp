#include "MarkSweepHeap.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../memory/Heap.h"
#include "../vm/Print.h"
#include "../vmobjects/AbstractObject.h"
#include "MarkSweepCollector.h"

// Objects larger than this use the large-object space; smaller ones are
// bucketed into size classes.
static const size_t MAX_SMALL_OBJECT_SIZE = 2048;
static const size_t ALIGNMENT = sizeof(void*);
static const size_t NUM_SIZE_CLASSES = MAX_SMALL_OBJECT_SIZE / ALIGNMENT;

static inline size_t sizeToClass(size_t size) {
    return (size - 1) / ALIGNMENT;
}
static inline size_t cellSizeForClass(size_t classIndex) {
    return (classIndex + 1) * ALIGNMENT;
}

// Page granularity; must be >= the largest size class.
static const size_t PAGE_SIZE = (size_t)32 * 1024;

static const size_t GC_UNMARKED = 0;

// Access a cell's gcfield via the non-virtual accessors, so the sweeper and the
// mark phase agree on its location and it works on a free (unconstructed) cell.
static inline size_t cellMark(void* cell) {
    return ((AbstractVMObject*)cell)->GetGCField();
}
static inline void setCellMark(void* cell, size_t value) {
    ((AbstractVMObject*)cell)->SetGCField(value);
}

size_t MarkSweepHeap::sizeClassIndex(size_t size) {
    return sizeToClass(size);
}

MarkSweepHeap::MarkSweepHeap(size_t objectSpaceSize)
    : Heap<MarkSweepHeap>(new MarkSweepCollector(this)),
      freeLists(NUM_SIZE_CLASSES, nullptr), classPages(NUM_SIZE_CLASSES),
      sweepCursor(NUM_SIZE_CLASSES, 0),
      collectionLimit((size_t)((double)objectSpaceSize * 0.9)),
      minCollectionLimit((size_t)((double)objectSpaceSize * 0.9)) {}

MarkSweepHeap::~MarkSweepHeap() {
    for (auto& pages : classPages) {
        for (auto* page : pages) {
            free(page->memory);
            delete page;
        }
    }
    for (auto* obj : largeObjects) {
        free((void*)obj);
    }
}

void MarkSweepHeap::accountAllocation(size_t bytes) {
    spcAlloc += bytes;
    if (spcAlloc >= collectionLimit) {
        requestGC();
    }
}

void MarkSweepHeap::carveNewPage(size_t classIndex) {
    size_t const cellSize = cellSizeForClass(classIndex);
    size_t const numCells = PAGE_SIZE / cellSize;

    char* memory = (char*)calloc(1, PAGE_SIZE);
    if (memory == nullptr) {
        ErrorPrint("\nFailed to allocate a " + to_string(PAGE_SIZE) +
                   " Byte heap page.\n");
        Quit(-1);
    }

    // Mark the page swept for this epoch: its cells are already free below, so
    // the sweeper skips it this cycle.
    auto* page = new Page{memory, epoch};
    classPages[classIndex].push_back(page);

    FreeListEntry* head = freeLists[classIndex];
    for (size_t i = 0; i < numCells; i++) {
        char* p = memory + (i * cellSize);
        // keep the unmarked invariant explicit so the sweeper reclaims it
        setCellMark(p, GC_UNMARKED);
        auto* cell = (FreeListEntry*)p;
        cell->next = head;
        head = cell;
    }
    freeLists[classIndex] = head;
}

bool MarkSweepHeap::sweepPageAt(size_t classIndex, size_t pageIndex) {
    auto& pages = classPages[classIndex];
    Page* page = pages[pageIndex];
    size_t const cellSize = cellSizeForClass(classIndex);
    size_t const numCells = PAGE_SIZE / cellSize;
    char* const base = page->memory;

    // Stage reclaimed cells on a local chain; only publish them if the page has
    // a live cell, so a fully-dead page can be freed without dangling entries.
    FreeListEntry* localHead = nullptr;
    FreeListEntry* localTail = nullptr;
    size_t liveCells = 0;

    for (size_t i = 0; i < numCells; i++) {
        char* p = base + (i * cellSize);
        if (cellMark(p) == epoch) {
            liveCells++;
            continue;
        }
        auto* cell = (FreeListEntry*)p;  // dead or already-free -> reclaim
        if (localTail == nullptr) {
            localTail = cell;
        }
        cell->next = localHead;
        localHead = cell;
    }

    if (liveCells == 0) {  // whole page is garbage -> return it to the OS
        free(page->memory);
        delete page;
        pages[pageIndex] = pages.back();
        pages.pop_back();
        return true;
    }

    if (localHead != nullptr) {
        localTail->next = freeLists[classIndex];
        freeLists[classIndex] = localHead;
    }
    page->sweptEpoch = epoch;
    return false;
}

bool MarkSweepHeap::sweepNextPage(size_t classIndex) {
    auto& pages = classPages[classIndex];
    while (sweepCursor[classIndex] < pages.size()) {
        size_t const idx = sweepCursor[classIndex];
        if (pages[idx]->sweptEpoch == epoch) {  // already swept this cycle
            sweepCursor[classIndex]++;
            continue;
        }
        if (sweepPageAt(classIndex, idx)) {
            continue;  // page freed; a different page is now at idx
        }
        sweepCursor[classIndex]++;
        if (freeLists[classIndex] != nullptr) {
            return true;
        }
    }
    return false;
}

AbstractVMObject* MarkSweepHeap::allocateLargeObject(size_t size) {
    auto* newObject = (AbstractVMObject*)malloc(size);
    if (newObject == nullptr) {
        ErrorPrint("\nFailed to allocate " + to_string(size) + " Bytes.\n");
        Quit(-1);
    }
    memset((void*)newObject, 0, size);
    largeObjects.push_back(newObject);

    accountAllocation(size);
    return newObject;
}

AbstractVMObject* MarkSweepHeap::AllocateObject(size_t size) {
    if (size > MAX_SMALL_OBJECT_SIZE) {
        return allocateLargeObject(size);
    }

    size_t const classIndex = sizeClassIndex(size);
    if (freeLists[classIndex] == nullptr) {
        // reclaim from a not-yet-swept page before growing the heap
        if (!sweepNextPage(classIndex)) {
            carveNewPage(classIndex);
        }
    }

    FreeListEntry* cell = freeLists[classIndex];
    freeLists[classIndex] = cell->next;

    // zero the requested bytes (also sets gcfield to GC_UNMARKED); padding
    // bytes are never read as fields
    memset((void*)cell, 0, size);

    accountAllocation(size);
    return (AbstractVMObject*)cell;
}
