#include "opus_x64_layout.h"

/*
 * AMD64 translations of the document-descriptor shortcuts in
 * Opus/asm/fetchn2.asm. Movable handles remain double-indirect, but native
 * pointers replace the original segment:offset address calculations.
 */

extern "C" void** mpdochdod[];

namespace {

constexpr int kDocNil = 0;

void* DodOrNull(const int doc) {
    void** handle = mpdochdod[doc];
    return handle == nullptr ? nullptr : *handle;
}

}  // namespace

extern "C" int DocMother(const int doc) {
    void* const pdod = DodOrNull(doc);
    if (pdod == nullptr) {
        return kDocNil;
    }
    return OpusDodIsMother(pdod) ? doc : OpusDodMotherDoc(pdod);
}

extern "C" void* N_PdodMother(const int doc) {
    void* pdod = DodOrNull(doc);
    if (pdod == nullptr || OpusDodIsMother(pdod)) {
        return pdod;
    }
    return DodOrNull(OpusDodMotherDoc(pdod));
}

extern "C" int DocDotMother(int doc) {
    for (;;) {
        void* const pdod = DodOrNull(doc);
        if (pdod == nullptr) {
            return kDocNil;
        }
        if (OpusDodIsDocument(pdod)) {
            return OpusDodTemplateDoc(pdod);
        }
        if (OpusDodIsTemplate(pdod)) {
            return doc;
        }
        doc = OpusDodMotherDoc(pdod);
    }
}
