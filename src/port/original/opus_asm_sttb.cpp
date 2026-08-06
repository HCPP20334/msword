#include <cstddef>

/* Direct AMD64 C++ translation of Opus/asm/sttbn.asm. */
extern "C" void AddDcbToLprgbst(int* offsets, const int count,
                                  const int delta, const int threshold) {
    if (offsets == nullptr || count <= 0) {
        return;
    }
    for (int index = 0; index < count; ++index) {
        if (offsets[index] >= threshold) {
            offsets[index] += delta;
        }
    }
}

