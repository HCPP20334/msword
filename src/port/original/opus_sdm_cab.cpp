#include "opus_x64_compat.h"
#include "opus_x64_heap.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

/* Native CAB storage for Microsoft's SDM 2.22 interface.
 *
 * The SDM implementation checked into this archive is 16-bit object code,
 * but its CAB layout and complete public contract are present in sdm.h and
 * sdmproc.h.  This file preserves that contract with flat movable handles.
 */

namespace {

using Word = std::uint16_t;
using CabHandle = void**;

struct CabHeader {
    Word simple_words;
    Word handle_words;
};

constexpr std::size_t kWordBytes = sizeof(Word);
constexpr std::size_t kCabMinWords =
    (sizeof(CabHeader) + sizeof(Word)) / kWordBytes;
constexpr std::size_t kPointerWords = sizeof(void*) / kWordBytes;
constexpr Word kNinch = 0xffff;

std::byte* cab_bytes(const CabHandle cab) noexcept {
    return cab == nullptr ? nullptr
                          : static_cast<std::byte*>(OpusDerefH(cab));
}

CabHeader* cab_header(const CabHandle cab) noexcept {
    return reinterpret_cast<CabHeader*>(cab_bytes(cab));
}

std::size_t cab_word_count(const CabHandle cab) noexcept {
    return OpusCbOfH(cab) / kWordBytes;
}

void*** handle_slot(const CabHandle cab, const Word argument_index) noexcept {
    auto* const header = cab_header(cab);
    if (header == nullptr || argument_index >= header->handle_words ||
        argument_index % kPointerWords != 0) {
        return nullptr;
    }
    return reinterpret_cast<void***>(
        cab_bytes(cab) + (kCabMinWords + argument_index) * kWordBytes);
}

void clear_cab_data(const CabHandle cab) noexcept {
    auto* const header = cab_header(cab);
    if (header == nullptr) {
        return;
    }
    for (Word index = 0; index < header->handle_words;
         index = static_cast<Word>(index + kPointerWords)) {
        void*** const slot = handle_slot(cab, index);
        if (slot != nullptr && *slot != nullptr) {
            OpusFreeH(*slot);
            *slot = nullptr;
        }
    }
}

bool initialize_cab(const CabHandle cab, const Word initializer) noexcept {
    auto* const bytes = cab_bytes(cab);
    if (bytes == nullptr) {
        return false;
    }

    const std::size_t total_words = initializer & 0x00ffu;
    const std::size_t handle_count = initializer >> 8u;
    if (total_words < kCabMinWords || total_words > cab_word_count(cab) ||
        handle_count * kPointerWords > total_words - kCabMinWords) {
        return false;
    }

    std::memset(bytes, 0, total_words * kWordBytes);
    auto* const header = reinterpret_cast<CabHeader*>(bytes);
    header->simple_words = static_cast<Word>(total_words - kCabMinWords);
    header->handle_words = static_cast<Word>(handle_count * kPointerWords);
    return true;
}

bool set_raw(const CabHandle cab, const void* source,
             const std::size_t byte_count, const Word argument_index) {
    void*** const slot = handle_slot(cab, argument_index);
    if (slot == nullptr || (source == nullptr && byte_count != 0)) {
        return false;
    }

    void** const replacement = OpusHAllocateCb(byte_count);
    if (replacement == nullptr) {
        return false;
    }
    if (byte_count != 0) {
        std::memcpy(OpusDerefH(replacement), source, byte_count);
    }

    if (*slot != nullptr) {
        OpusFreeH(*slot);
    }
    *slot = replacement;
    return true;
}

const unsigned char* raw_data(const CabHandle cab,
                              const Word argument_index) noexcept {
    void*** const slot = handle_slot(cab, argument_index);
    return slot == nullptr || *slot == nullptr
               ? nullptr
               : static_cast<const unsigned char*>(OpusDerefH(*slot));
}

std::size_t raw_size(const CabHandle cab,
                     const Word argument_index) noexcept {
    void*** const slot = handle_slot(cab, argument_index);
    return slot == nullptr || *slot == nullptr ? 0 : OpusCbOfH(*slot);
}

void copy_sz(const CabHandle cab, char* destination,
             const Word destination_bytes, const Word argument_index) {
    if (destination == nullptr || destination_bytes == 0) {
        return;
    }
    const auto* const source = reinterpret_cast<const char*>(
        raw_data(cab, argument_index));
    if (source == nullptr) {
        destination[0] = '\0';
        return;
    }
    const std::size_t source_size = raw_size(cab, argument_index);
    const std::size_t source_length =
        source_size == 0 ? 0 : strnlen(source, source_size);
    const std::size_t count = (std::min)(
        source_length, static_cast<std::size_t>(destination_bytes - 1));
    std::memcpy(destination, source, count);
    destination[count] = '\0';
}

}  // namespace

extern "C" {

CabHandle hcabDlgCur = nullptr;
Word wRefDlgCur = 0;

CabHandle HcabAlloc_sdm21(const Word initializer) {
    const std::size_t total_words = initializer & 0x00ffu;
    if (total_words < kCabMinWords) {
        return nullptr;
    }
    CabHandle const cab = OpusHAllocateCb(total_words * kWordBytes);
    if (cab == nullptr || !initialize_cab(cab, initializer)) {
        OpusFreeH(cab);
        return nullptr;
    }
    return cab;
}

void InitCab_sdm21(const CabHandle cab, const Word initializer) {
    if (cab == nullptr) {
        return;
    }
    clear_cab_data(cab);
    initialize_cab(cab, initializer);
}

void ReinitCab(const CabHandle cab, const Word initializer) {
    InitCab_sdm21(cab, initializer);
}

void FreeCabData(const CabHandle cab) { clear_cab_data(cab); }

void FreeCab(const CabHandle cab) {
    clear_cab_data(cab);
    OpusFreeH(cab);
}

void* PcabLockCab(const CabHandle cab) { return cab_bytes(cab); }
void UnlockCab(CabHandle) {}

void NinchCab(const CabHandle cab) {
    auto* const header = cab_header(cab);
    if (header == nullptr) {
        return;
    }
    clear_cab_data(cab);
    auto* words = reinterpret_cast<Word*>(cab_bytes(cab));
    for (Word index = header->handle_words; index < header->simple_words;
         ++index) {
        words[kCabMinWords + index] = kNinch;
    }
}

int FSetCabRgb(const CabHandle cab, const char* source, const Word byte_count,
               const Word argument_index) {
    return set_raw(cab, source, byte_count, argument_index);
}

void GetCabRgb(const CabHandle cab, char* destination,
               const Word byte_count, const Word argument_index) {
    if (destination == nullptr || byte_count == 0) {
        return;
    }
    const std::size_t count = (std::min)(
        static_cast<std::size_t>(byte_count), raw_size(cab, argument_index));
    const auto* const source = raw_data(cab, argument_index);
    if (source != nullptr && count != 0) {
        std::memcpy(destination, source, count);
    }
    if (count < byte_count) {
        std::memset(destination + count, 0, byte_count - count);
    }
}

int FSetCabSz(const CabHandle cab, const char* source,
              const Word argument_index) {
    if (source == nullptr) {
        return false;
    }
    return set_raw(cab, source, std::strlen(source) + 1, argument_index);
}

void GetCabSz(const CabHandle cab, char* destination,
              const Word destination_bytes, const Word argument_index) {
    copy_sz(cab, destination, destination_bytes, argument_index);
}

int FSetCabSt(const CabHandle cab, const unsigned char* source,
              const Word argument_index) {
    if (source == nullptr) {
        return false;
    }
    const std::size_t length = source[0];
    void*** const slot = handle_slot(cab, argument_index);
    if (slot == nullptr) {
        return false;
    }
    void** const replacement = OpusHAllocateCb(length + 1);
    if (replacement == nullptr) {
        return false;
    }
    auto* const destination = static_cast<unsigned char*>(
        OpusDerefH(replacement));
    std::memcpy(destination, source + 1, length);
    destination[length] = 0;
    if (*slot != nullptr) {
        OpusFreeH(*slot);
    }
    *slot = replacement;
    return true;
}

void GetCabSt(const CabHandle cab, unsigned char* destination,
              const Word destination_bytes, const Word argument_index) {
    if (destination == nullptr || destination_bytes == 0) {
        return;
    }
    const auto* const source = raw_data(cab, argument_index);
    const std::size_t size = raw_size(cab, argument_index);
    const std::size_t source_length = source == nullptr || size == 0
                                          ? 0
                                          : strnlen(
                                                reinterpret_cast<const char*>(source),
                                                size);
    const std::size_t count = (std::min)(
        source_length, static_cast<std::size_t>(destination_bytes - 1));
    destination[0] = static_cast<unsigned char>((std::min)(
        count, static_cast<std::size_t>(
                   (std::numeric_limits<unsigned char>::max)())));
    if (count != 0) {
        std::memcpy(destination + 1, source, count);
    }
}

void GetCabStz(const CabHandle cab, unsigned char* destination,
               const Word destination_bytes, const Word argument_index) {
    GetCabSt(cab, destination, destination_bytes, argument_index);
    if (destination != nullptr && destination_bytes > 1) {
        const std::size_t terminator = (std::min)(
            static_cast<std::size_t>(destination[0]) + 1,
            static_cast<std::size_t>(destination_bytes - 1));
        destination[terminator] = 0;
    }
}

CabHandle HcabDupeCab(const CabHandle source) {
    const auto* const source_header = cab_header(source);
    if (source_header == nullptr) {
        return nullptr;
    }
    const std::size_t total_words = cab_word_count(source);
    if (total_words > 0xffu ||
        source_header->handle_words % kPointerWords != 0) {
        return nullptr;
    }
    const Word initializer = static_cast<Word>(
        total_words |
        ((source_header->handle_words / kPointerWords) << 8u));
    CabHandle const duplicate = HcabAlloc_sdm21(initializer);
    if (duplicate == nullptr) {
        return nullptr;
    }

    auto* const destination_header = cab_header(duplicate);
    auto* const source_words = reinterpret_cast<const Word*>(cab_bytes(source));
    auto* const destination_words = reinterpret_cast<Word*>(cab_bytes(duplicate));
    for (Word index = source_header->handle_words;
         index < source_header->simple_words; ++index) {
        destination_words[kCabMinWords + index] =
            source_words[kCabMinWords + index];
    }

    for (Word index = 0; index < source_header->handle_words;
         index = static_cast<Word>(index + kPointerWords)) {
        void*** const source_slot = handle_slot(source, index);
        if (source_slot != nullptr && *source_slot != nullptr &&
            !set_raw(duplicate, OpusDerefH(*source_slot),
                     OpusCbOfH(*source_slot), index)) {
            FreeCab(duplicate);
            return nullptr;
        }
    }
    destination_header->simple_words = source_header->simple_words;
    destination_header->handle_words = source_header->handle_words;
    return duplicate;
}

}  // extern "C"
