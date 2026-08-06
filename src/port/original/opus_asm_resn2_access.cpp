#include "opus_x64_layout.h"

/* AMD64 translation of Opus/asm/resn2.asm:N_PmwdWw. */

extern "C" void** mpwwhwwd[];
extern "C" void** mpmwhmwd[];

namespace {

void* DescriptorFromTable(void*** table, const int index) {
    void** handle = table[index];
    return handle == nullptr ? nullptr : *handle;
}

}  // namespace

extern "C" void* N_PmwdWw(const int ww) {
    void* const pwwd = DescriptorFromTable(mpwwhwwd, ww);
    if (pwwd == nullptr) {
        return nullptr;
    }
    return DescriptorFromTable(mpmwhmwd, OpusWwdMw(pwwd));
}
