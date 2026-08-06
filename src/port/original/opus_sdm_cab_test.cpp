#include "opus_x64_compat.h"
#include "opus_x64_heap.h"

#include <cstdint>
#include <cstring>

namespace {

using Word = std::uint16_t;
using CabHandle = void**;

extern "C" {
CabHandle HcabAlloc_sdm21(Word);
void FreeCab(CabHandle);
void NinchCab(CabHandle);
int FSetCabSz(CabHandle, const char*, Word);
void GetCabSz(CabHandle, char*, Word, Word);
int FSetCabSt(CabHandle, const unsigned char*, Word);
void GetCabStz(CabHandle, unsigned char*, Word, Word);
CabHandle HcabDupeCab(CabHandle);
}

constexpr Word kCabMinWords = 3;
constexpr Word kPointerWords = sizeof(void*) / sizeof(Word);
constexpr Word kHandleBaseIag =
    ((kCabMinWords + kPointerWords - 1) / kPointerWords * kPointerWords) -
    kCabMinWords;
constexpr Word kHandleCount = 2;
constexpr Word kArgumentWords =
    kHandleBaseIag + kHandleCount * kPointerWords + 2;
constexpr Word kTotalWords = kCabMinWords + kArgumentWords;
constexpr Word kInitializer = kTotalWords | (kHandleCount << 8);

int fail(const int line) { return line; }

}  // namespace

int main() {
    const std::size_t initial_heap = OpusHeapBytesUsed();
    CabHandle const cab = HcabAlloc_sdm21(kInitializer);
    if (cab == nullptr) {
        return fail(__LINE__);
    }

    const auto* words = static_cast<const Word*>(OpusDerefH(cab));
    if (words[0] != kArgumentWords ||
        words[1] != kHandleCount * kPointerWords) {
        return fail(__LINE__);
    }

    if (!FSetCabSz(cab, "Alpha", kHandleBaseIag)) {
        return fail(__LINE__);
    }
    const unsigned char counted[] = {4, 'B', 'e', 't', 'a'};
    if (!FSetCabSt(cab, counted, kHandleBaseIag + kPointerWords)) {
        return fail(__LINE__);
    }

    char sz[16]{};
    GetCabSz(cab, sz, sizeof(sz), kHandleBaseIag);
    if (std::strcmp(sz, "Alpha") != 0) {
        return fail(__LINE__);
    }
    unsigned char stz[16]{};
    GetCabStz(cab, stz, sizeof(stz), kHandleBaseIag + kPointerWords);
    if (stz[0] != 4 || std::memcmp(stz + 1, "Beta", 5) != 0) {
        return fail(__LINE__);
    }

    auto* mutable_words = static_cast<Word*>(OpusDerefH(cab));
    mutable_words[kCabMinWords + kHandleBaseIag +
                  kHandleCount * kPointerWords] = 42;
    CabHandle const duplicate = HcabDupeCab(cab);
    if (duplicate == nullptr) {
        return fail(__LINE__);
    }
    GetCabSz(duplicate, sz, sizeof(sz), kHandleBaseIag);
    if (std::strcmp(sz, "Alpha") != 0 ||
        static_cast<Word*>(OpusDerefH(duplicate))
                [kCabMinWords + kHandleBaseIag +
                 kHandleCount * kPointerWords] != 42) {
        return fail(__LINE__);
    }

    NinchCab(cab);
    GetCabSz(cab, sz, sizeof(sz), kHandleBaseIag);
    mutable_words = static_cast<Word*>(OpusDerefH(cab));
    if (sz[0] != '\0' ||
        mutable_words[kCabMinWords + kHandleBaseIag +
                      kHandleCount * kPointerWords] !=
            0xffffu) {
        return fail(__LINE__);
    }

    FreeCab(duplicate);
    FreeCab(cab);
    return OpusHeapBytesUsed() == initial_heap ? 0 : fail(__LINE__);
}
