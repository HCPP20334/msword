#include "opus_x64_layout.h"

#include <cstddef>

/* AMD64 translation of Opus/asm/resn2.asm:HpInPl. */

extern "C" unsigned char* HpInPl(void** hpl, const int index) {
    if (hpl == nullptr || *hpl == nullptr) {
        return nullptr;
    }
    auto* const base = static_cast<unsigned char*>(OpusPlData(*hpl));
    return base + static_cast<std::size_t>(index) *
                      static_cast<std::size_t>(OpusPlEntrySize(*hpl));
}

extern "C" unsigned char* PInPl(void** hpl, const int index) {
    return HpInPl(hpl, index);
}
