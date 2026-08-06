#include <cstddef>

/*
 * AMD64 translation of Opus/asm/resn.asm:LRghEngine and its four exported
 * descriptor-table entry points. The original segment/word address
 * calculation becomes ordinary native array indexing; the double-indirect
 * movable-handle dereference remains unchanged.
 */

extern "C" void** mpdochdod[];
extern "C" void** mpfnhfcb[];
extern "C" void** mpwwhwwd[];
extern "C" void** mpmwhmwd[];

namespace {

void* DescriptorFromTable(void*** table, const int index) {
    void** handle = table[index];
    return handle == nullptr ? nullptr : *handle;
}

}  // namespace

extern "C" void* N_PdodDoc(const int doc) {
    return DescriptorFromTable(mpdochdod, doc);
}

extern "C" void* N_PfcbFn(const int fn) {
    return DescriptorFromTable(mpfnhfcb, fn);
}

extern "C" void* N_PwwdWw(const int ww) {
    return DescriptorFromTable(mpwwhwwd, ww);
}

extern "C" void** N_HwwdWw(const int ww) {
    return mpwwhwwd[ww];
}

extern "C" void* N_PmwdMw(const int mw) {
    return DescriptorFromTable(mpmwhmwd, mw);
}
