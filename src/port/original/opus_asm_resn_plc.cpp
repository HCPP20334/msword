#include "opus_x64_layout.h"

#include <cstddef>
#include <cstring>

/*
 * AMD64 translations of the foundational PLC access routines in resn.asm
 * and fetchn.asm. A PLC still uses the original packed header and movable
 * handle; only its segmented pointer calculations become native pointers.
 */

extern "C" struct OpusFastPlcCache {
    void** hplc;
    unsigned char* pchFoo;
    int cbFoo;
    unsigned int bfoo;
} vfpc;

namespace {

long* CpData(void* pplc) {
    return static_cast<long*>(OpusPlcCpData(pplc));
}

long AdjustmentAt(const void* pplc, const int index) {
    return index < OpusPlcAdjustIndex(pplc) ? 0L : OpusPlcAdjustment(pplc);
}

unsigned char* FooAt(void* pplc, const int index) {
    auto* const cpData = reinterpret_cast<unsigned char*>(CpData(pplc));
    const std::size_t cpBytes = static_cast<std::size_t>(OpusPlcIMax(pplc)) *
                                sizeof(long);
    const std::size_t fooBytes = static_cast<std::size_t>(index) *
                                 static_cast<std::size_t>(OpusPlcEntrySize(pplc));
    return cpData + cpBytes + fooBytes;
}

}  // namespace

extern "C" long* LprgcpForPlc(void* pplc) {
    return CpData(pplc);
}

extern "C" long CpPlc(void** hplc, const int index) {
    void* const pplc = *hplc;
    return CpData(pplc)[index] + AdjustmentAt(pplc, index);
}

extern "C" int PutCpPlc(void** hplc, const int index, const long cp) {
    void* const pplc = *hplc;
    CpData(pplc)[index] = cp - AdjustmentAt(pplc, index);
    return 0;
}

extern "C" int GetPlc(void** hplc, const int index, unsigned char* data) {
    void* const pplc = *hplc;
    const int byteCount = OpusPlcEntrySize(pplc);
    unsigned char* const source = FooAt(pplc, index);
    if (byteCount > 0) {
        std::memmove(data, source, static_cast<std::size_t>(byteCount));
    }
    vfpc.hplc = hplc;
    vfpc.pchFoo = data;
    vfpc.cbFoo = byteCount;
    vfpc.bfoo = static_cast<unsigned int>(
        source - reinterpret_cast<unsigned char*>(CpData(pplc)));
    return 0;
}

extern "C" int PutPlc(void** hplc, const int index,
                        const unsigned char* data) {
    void* const pplc = *hplc;
    const int byteCount = OpusPlcEntrySize(pplc);
    if (byteCount > 0) {
        std::memmove(FooAt(pplc, index), data,
                     static_cast<std::size_t>(byteCount));
    }
    return 0;
}

extern "C" int PutPlcLastProc() {
    if (vfpc.hplc == nullptr || *vfpc.hplc == nullptr ||
        vfpc.pchFoo == nullptr || vfpc.cbFoo <= 0) {
        return 0;
    }
    auto* const base = reinterpret_cast<unsigned char*>(CpData(*vfpc.hplc));
    std::memmove(base + vfpc.bfoo, vfpc.pchFoo,
                 static_cast<std::size_t>(vfpc.cbFoo));
    return 0;
}

extern "C" int IInPlc(void** hplc, const long cp) {
    void* const pplc = *hplc;
    const int last = OpusPlcIMac(pplc);
    int low = 0;
    int high = last;
    while (low < high) {
        const int middle = low + (high - low + 1) / 2;
        const long middleCp = CpData(pplc)[middle] + AdjustmentAt(pplc, middle);
        if (middleCp <= cp) {
            low = middle;
        } else {
            high = middle - 1;
        }
    }
    if (OpusPlcHasMultipleCps(pplc)) {
        while (low > 0 && CpData(pplc)[low - 1] + AdjustmentAt(pplc, low - 1) ==
                              CpData(pplc)[low] + AdjustmentAt(pplc, low)) {
            --low;
        }
    }
    OpusPlcSetHint(pplc, low);
    return low;
}

extern "C" int IInPlcCheck(void** hplc, const long cp) {
    if (hplc == nullptr || *hplc == nullptr) {
        return -1;
    }
    const int last = OpusPlcIMac(*hplc);
    if (cp < CpPlc(hplc, 0) || cp >= CpPlc(hplc, last)) {
        return -1;
    }
    return IInPlc(hplc, cp);
}

extern "C" int IInPlcRef(void** hplc, const long cp) {
    if (hplc == nullptr || *hplc == nullptr) {
        return -1;
    }
    const int last = OpusPlcIMac(*hplc);
    if (CpPlc(hplc, 0) >= cp) {
        return 0;
    }
    if (CpPlc(hplc, last) < cp) {
        return -1;
    }
    int index = IInPlc(hplc, cp);
    while (index <= last && CpPlc(hplc, index) < cp) {
        ++index;
    }
    return index;
}

extern "C" int IMacPlc(void** hplc) {
    return hplc == nullptr || *hplc == nullptr ? 0 : OpusPlcIMac(*hplc);
}

extern "C" int CompletelyAdjustHplcCps(void** hplc) {
    if (hplc == nullptr || *hplc == nullptr) {
        return 0;
    }
    void* const pplc = *hplc;
    const long adjustment = OpusPlcAdjustment(pplc);
    if (adjustment != 0) {
        long* const cps = CpData(pplc);
        for (int index = OpusPlcAdjustIndex(pplc);
             index <= OpusPlcIMac(pplc); ++index) {
            cps[index] += adjustment;
        }
    }
    OpusPlcClearAdjustment(pplc);
    return 0;
}

extern "C" int AddDcpToCps(long* cps, const int first, const int limit,
                            const long adjustment) {
    for (int index = first; index < limit; ++index) {
        cps[index] += adjustment;
    }
    return 0;
}

extern "C" int AdjustHplcCpsToLim(void** hplc, const int limit) {
    if (hplc == nullptr || *hplc == nullptr ||
        OpusPlcAdjustment(*hplc) == 0) {
        return 0;
    }
    void* const pplc = *hplc;
    AddDcpToCps(CpData(pplc), OpusPlcAdjustIndex(pplc), limit,
                OpusPlcAdjustment(pplc));
    OpusPlcSetAdjustIndex(pplc, limit);
    if (limit == OpusPlcIMac(pplc) + 1) {
        OpusPlcClearAdjustment(pplc);
    }
    return 0;
}

extern "C" int BltInPlc(const int mode, void** hplc, const unsigned int index,
                         const int cp_delta, const int foo_delta,
                         const unsigned int count) {
    void* const pplc = *hplc;
    auto* const cps = reinterpret_cast<unsigned char*>(CpData(pplc));
    unsigned char* source;
    std::ptrdiff_t byte_delta;
    std::size_t byte_count;
    if (mode == 0) {
        source = cps + static_cast<std::size_t>(index) * sizeof(long);
        byte_delta = static_cast<std::ptrdiff_t>(cp_delta) * sizeof(long);
        byte_count = static_cast<std::size_t>(count) * sizeof(long);
    } else {
        const std::size_t entry_size =
            static_cast<std::size_t>(OpusPlcEntrySize(pplc));
        source = cps + static_cast<std::size_t>(OpusPlcIMax(pplc)) *
                         sizeof(long) +
                 static_cast<std::size_t>(index) * entry_size;
        byte_delta = static_cast<std::ptrdiff_t>(cp_delta) * sizeof(long) +
                     static_cast<std::ptrdiff_t>(foo_delta) * entry_size;
        byte_count = static_cast<std::size_t>(count) * entry_size;
    }
    if (byte_count != 0) {
        std::memmove(source + byte_delta, source, byte_count);
    }
    return 0;
}
