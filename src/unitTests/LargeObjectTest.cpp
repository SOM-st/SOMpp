#include "LargeObjectTest.h"

#include <cppunit/TestAssert.h>
#include <cstddef>
#include <string>

#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../vm/Globals.h"
#include "../vm/Symbols.h"
#include "../vm/Universe.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMArray.h"
#include "../vmobjects/VMObjectBase.h"  // NOLINT(misc-include-cleaner)
#include "../vmobjects/VMString.h"

// default heap size for tests
static const size_t heapSize = 1024UL * 1024;

static void fillHeapUntilGCIsRequested() {
    while (!GetHeap<HEAP_CLS>()->isCollectionTriggered()) {
        Universe::NewArray(100);
    }
}

void LargeObjectTest::testLargeObjectsSurviveGC() {
    GetHeap<HEAP_CLS>()->FullGC();
    fillHeapUntilGCIsRequested();

    // array of half heap size
    size_t const arraySize = heapSize / 2 / sizeof(vm_oop_t);
    VMArray* arr = Universe::NewArray(arraySize);
    arr->SetIndexableField(0, Universe::NewString("first"));
    arr->SetIndexableField(arraySize - 1, Universe::NewString("last"));

    // string larger than the heap
    std::string const largeStr(4 * heapSize, 'a');
    VMString* str = Universe::NewString(largeStr);

    Universe::SetGlobal(SymbolFor("largeObjectTestArray"), arr);
    Universe::SetGlobal(SymbolFor("largeObjectTestString"), str);

    GetHeap<HEAP_CLS>()->FullGC();

    // things may have moved
    VMSymbol* arrName = SymbolFor("largeObjectTestArray");
    VMSymbol* strName = SymbolFor("largeObjectTestString");

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    arr = static_cast<VMArray*>(Universe::GetGlobal(arrName));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    str = static_cast<VMString*>(Universe::GetGlobal(strName));

    CPPUNIT_ASSERT_EQUAL(arraySize, arr->GetNumberOfIndexableFields());
    // NOLINTBEGIN(cppcoreguidelines-pro-type-static-cast-downcast)
    CPPUNIT_ASSERT_EQUAL(
        std::string("first"),
        static_cast<VMString*>(arr->GetIndexableField(0))->GetStdString());
    CPPUNIT_ASSERT_EQUAL(
        std::string("last"),
        static_cast<VMString*>(arr->GetIndexableField(arraySize - 1))
            ->GetStdString());
    // NOLINTEND(cppcoreguidelines-pro-type-static-cast-downcast)
    CPPUNIT_ASSERT(largeStr == str->GetStdString());

    Universe::SetGlobal(arrName, load_ptr(nilObject));
    Universe::SetGlobal(strName, load_ptr(nilObject));
}

void LargeObjectTest::testLargeObjectsRequestGC() {
    GetHeap<HEAP_CLS>()->FullGC();
    CPPUNIT_ASSERT(!GetHeap<HEAP_CLS>()->isCollectionTriggered());

    size_t const arraySize = heapSize / 2 / sizeof(vm_oop_t);
    for (size_t i = 0; i < 100; i += 1) {
        Universe::NewArray(arraySize);
        if (GetHeap<HEAP_CLS>()->isCollectionTriggered()) {
            return;
        }
    }
    CPPUNIT_FAIL("allocating only large objects did not request a GC");
}

#if GC_TYPE == GENERATIONAL

static void assertIsMature(AbstractVMObject* obj) {
    CPPUNIT_ASSERT(!GetHeap<HEAP_CLS>()->IsObjectInNursery(obj));
    CPPUNIT_ASSERT((obj->GetGCField() & MASK_OBJECT_IS_OLD) != 0);
}

void LargeObjectTest::testTooLargeForNurseryGoesToMatureSpace() {
    std::string const large(4 * heapSize, 'a');
    assertIsMature(Universe::NewString(large));
}

void LargeObjectTest::testFullNurseryAllocatesInMatureSpace() {
    GetHeap<HEAP_CLS>()->FullGC();

    VMString* young = Universe::NewString("young");
    CPPUNIT_ASSERT(GetHeap<HEAP_CLS>()->IsObjectInNursery(young));

    // fill the nursery
    VMArray* arr = nullptr;
    do {
        arr = Universe::NewArray(1000);
    } while (GetHeap<HEAP_CLS>()->IsObjectInNursery(arr));
    assertIsMature(arr);

    // the nursery is now too full
    std::string const str(9000, 'a');
    assertIsMature(Universe::NewString(str));
    assertIsMature(Universe::NewArray(1000));

    // old objects with references to young ones need to be remembered
    arr->SetIndexableField(0, young);
    CPPUNIT_ASSERT((arr->GetGCField() & MASK_SEEN_BY_WRITE_BARRIER) != 0);
}

#endif
