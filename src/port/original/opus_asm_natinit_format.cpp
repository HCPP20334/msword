#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

/* Small native entry points translated from NATINIT.ASM and FORMATN.ASM. */
extern "C" {

extern int vwWinVersion;
extern int vfLargeFrameEMS;
extern int vfSmallFrameEMS;
extern int vfSingleApp;
extern int vcbitSegmentShift;

int C_DxpFromCh(int character, void* font_widths);

void FillBytePattern(unsigned char* destination, const unsigned char* pattern,
                     const int pattern_count, int byte_count) {
    if (destination == nullptr || pattern == nullptr ||
        pattern_count <= 0 || byte_count <= 0) {
        return;
    }
    while (byte_count > 0) {
        const int copied = (std::min)(pattern_count, byte_count);
        std::memcpy(destination, pattern, static_cast<std::size_t>(copied));
        destination += copied;
        byte_count -= copied;
    }
}

int FNativeInit() {
    /* Selector shifts and Win16 EMS frame modes disappear in a flat AMD64
     * process.  Keep the original observable initialization globals. */
    vfSingleApp = 0;
    vfLargeFrameEMS = 0;
    vfSmallFrameEMS = 0;
    vcbitSegmentShift = 0;
    vwWinVersion = 0x0A00;  /* native platform is newer than Win 3.x */
    return 1;
}

void GlobalMemInit() {}

int DxpFromCh(const int character, void* font_widths) {
    return C_DxpFromCh(character, font_widths);
}

int DxpFromChNat(const int character, void* font_widths) {
    return C_DxpFromCh(character, font_widths);
}

unsigned int UMultDivNR(const unsigned int value,
                        const unsigned int numerator,
                        const unsigned int denominator) {
    if (denominator == 0) {
        return 0xffffu;
    }
    const std::uint64_t result =
        static_cast<std::uint64_t>(value) * numerator / denominator;
    return result > 0xffffu ? 0xffffu : static_cast<unsigned int>(result);
}

}  // extern "C"
