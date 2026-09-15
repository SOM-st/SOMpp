#pragma once

#include "../misc/defs.h"
#include "../vmobjects/AbstractObject.h"
#include "GarbageCollector.h"

class VMFreeListEntry : public AbstractVMObject {
public:
    size_t size;
    VMFreeListEntry* next;

    VMFreeListEntry(size_t size, VMFreeListEntry* next)
        : size(size), next(next) {}

    bool IsEntryDirectlyFollowedBy(AbstractVMObject* other) {
        // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
        return (reinterpret_cast<size_t>(this) + this->size) ==
               // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
               reinterpret_cast<size_t>(other);
    }

    /** Size in bytes of the object. */
    [[nodiscard]] inline size_t GetObjectSize() const override { return size; }

    [[nodiscard]] inline int64_t GetHash() const override {
        ErrorPrint("this object doesn't support GetHash\n");
        return -1;
    }

    [[nodiscard]] inline VMClass* GetClass() const override {
        ErrorPrint("this object doesn't support GetClass\n");
        return nullptr;
    }

    [[nodiscard]] inline AbstractVMObject* CloneForMovingGC() const override {
        ErrorPrint("this object doesn't support CloneForMovingGC\n");
        return nullptr;
    }

    void MarkObjectAsInvalid() override {
        ErrorPrint("this object doesn't support MarkObjectAsInvalid\n");
    }

    [[nodiscard]] inline bool IsMarkedInvalid() const override {
        ErrorPrint("this object doesn't support IsMarkedInvalid\n");
        return false;
    }

    [[nodiscard]] inline std::string AsDebugString() const override {
        ErrorPrint("this object doesn't support AsDebugString\n");
        return "";
    }

    void* operator new(size_t size, AbstractVMObject* unusedObject) {
        assert(size >= sizeof(VMFreeListEntry));
        assert(unusedObject != nullptr);
        return static_cast<void*>(unusedObject);
    }

    [[nodiscard]] inline bool LargeEnoughForSplitting(size_t allocSize) const {
        return this->size >=
               allocSize +
                   sizeof(VMFreeListEntry);  // This is supposed to be
                                             // MIN_ALLOC_SIZE but C++...
    }

    static void AssertVMObjectStructure() {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
        static_assert(
            offsetof(VMFreeListEntry, gcField) == 8,
            "VMFreeListEntry must have the same layout as AbstractVMObject");
#pragma GCC diagnostic pop
    }
};

class MarkSweepHeap;
class MarkSweepCollector : public GarbageCollector<MarkSweepHeap> {
public:
    explicit MarkSweepCollector(MarkSweepHeap* heap) : GarbageCollector(heap) {}
    void Collect() override;

    static constexpr size_t MIN_ALLOC_SIZE = sizeof(VMFreeListEntry);

    inline static size_t GetAllocationSize(size_t requestedSize) {
        return max(requestedSize, MIN_ALLOC_SIZE);
    }

private:
    void markReachableObjects();
};
