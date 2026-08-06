#include <algorithm>
#include <cstdint>
#include <cstring>

/* AMD64 translation of PROMPTN.ASM.  Pointer members use their native size;
 * the scalar field ordering is identical to prompt.h::PPR. */
struct NativeProgressReport {
    int ich;
    int cch;
    unsigned int nLast;
    unsigned int nIncr;
    int abort_check;
    NativeProgressReport** previous;
    char** previous_text;
    short x_pixels;
};

extern "C" {

extern NativeProgressReport** vhpprPRPrompt;
int PopToHppr(NativeProgressReport** report);
int AdjustPrompt(int first_character, int character_count, int x_pixels,
                 char* replacement);

void ChangeProgressReport(NativeProgressReport** report,
                          unsigned int new_value) {
    if (report == nullptr) {
        return;
    }
    if (report != vhpprPRPrompt) {
        (void)PopToHppr(report);
    }
    NativeProgressReport* current = *report;
    if (current == nullptr || current->nIncr == 0) {
        return;
    }

    const unsigned int increment = current->nIncr;
    new_value = ((new_value + increment / 2u) / increment) * increment;
    if (new_value == current->nLast) {
        return;
    }
    current->nLast = new_value;

    char digits[6];
    std::memset(digits, ' ', sizeof(digits));
    const int width = (std::max)(0, (std::min)(current->cch, 6));
    unsigned int remaining = new_value;
    int position = 6;
    do {
        if (position <= 6 - width) {
            break;
        }
        digits[--position] =
            static_cast<char>('0' + static_cast<int>(remaining % 10u));
        remaining /= 10u;
    } while (remaining != 0);

    char* replacement = digits + (6 - width);
    (void)AdjustPrompt(current->ich, width, current->x_pixels, replacement);
}

void ProgressReportPercent(NativeProgressReport** report,
                           const long low, const long high,
                           const long value, long* next_value) {
    if (report == nullptr || *report == nullptr || high <= low ||
        (*report)->nIncr == 0) {
        return;
    }

    const std::int64_t range =
        static_cast<std::int64_t>(high) - static_cast<std::int64_t>(low);
    const unsigned int increment = (*report)->nIncr;
    const unsigned int partitions = 100u / increment;
    if (partitions == 0) {
        return;
    }

    const std::int64_t offset =
        static_cast<std::int64_t>(value) - static_cast<std::int64_t>(low);
    std::int64_t part =
        (offset * partitions + range / 2) / range;
    part = (std::max)(std::int64_t{0},
                      (std::min)(part, static_cast<std::int64_t>(partitions)));
    ChangeProgressReport(report,
        static_cast<unsigned int>(part) * increment);

    if (next_value != nullptr) {
        const std::int64_t next_partition =
            static_cast<std::int64_t>((*report)->nLast / increment) + 1;
        const std::int64_t numerator =
            range * next_partition - range / 2 + partitions - 1;
        *next_value = static_cast<long>(numerator / partitions + low);
    }
}

}  // extern "C"
