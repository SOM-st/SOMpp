#include "Symbols.h"

#include <cstddef>
#include <map>
#include <string>

#include "../memory/Heap.h"
#include "../misc/defs.h"
#include "../vmobjects/ObjectFormats.h"
#include "../vmobjects/VMSymbol.h"
#include "LogAllocation.h"

static map<std::string, GCSymbol*> symbolsMap;

GCSymbol* symbolSelf;
GCSymbol* symbolSuper;
GCSymbol* symbolBlockSelf;

GCSymbol* symbolIfTrue;
GCSymbol* symbolIfFalse;

GCSymbol* symbolPlus;
GCSymbol* symbolMinus;

VMSymbol* NewSymbol(const size_t length, const char* str) {
    auto* result =
        new (GetHeap<HEAP_CLS>(), PADDED_SIZE(length)) VMSymbol(length, str);
    symbolsMap[std::string(str, length)] = store_root(result);

    LOG_ALLOCATION("VMSymbol", result->GetObjectSize());
    return result;
}

VMSymbol* NewSymbol(const std::string& str) {
    return NewSymbol(str.length(), str.c_str());
}

VMSymbol* SymbolFor(const std::string& str) {
    auto const it = symbolsMap.find(str);
    return (it == symbolsMap.end()) ? NewSymbol(str) : load_ptr(it->second);
}

void InitializeSymbols() {
    symbolSelf = store_root(SymbolFor("self"));
    symbolSuper = store_root(SymbolFor("super"));
    symbolBlockSelf = store_root(SymbolFor("$blockSelf"));
    symbolIfTrue = store_root(SymbolFor("ifTrue:"));
    symbolIfFalse = store_root(SymbolFor("ifFalse:"));

    symbolPlus = store_root(SymbolFor("+"));
    symbolMinus = store_root(SymbolFor("-"));
}

void WalkSymbols(walk_heap_fn walk) {
    // walk all entries in symbols map
    map<std::string, GCSymbol*>::iterator symbolIter;
    for (symbolIter = symbolsMap.begin(); symbolIter != symbolsMap.end();
         symbolIter++) {
        // insert overwrites old entries inside the internal map
        symbolIter->second = static_cast<GCSymbol*>(walk(symbolIter->second));
    }

    // reassign symbols
    symbolSelf = static_cast<GCSymbol*>(walk(symbolSelf));
    symbolSuper = static_cast<GCSymbol*>(walk(symbolSuper));
    symbolBlockSelf = static_cast<GCSymbol*>(walk(symbolBlockSelf));
    symbolIfTrue = static_cast<GCSymbol*>(walk(symbolIfTrue));
    symbolIfFalse = static_cast<GCSymbol*>(walk(symbolIfFalse));

    symbolPlus = static_cast<GCSymbol*>(walk(symbolPlus));
    symbolMinus = static_cast<GCSymbol*>(walk(symbolMinus));
}
