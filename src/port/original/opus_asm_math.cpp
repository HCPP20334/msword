#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <limits>

/*
 * AMD64 translation of the accumulator boundary supplied by Word's 8087
 * math package and the transcendental portion of NATTRANS.ASM.  The public
 * NUM representation used by the original C code is an IEEE-754 double, so
 * the segmented accumulator can be represented directly without changing
 * mathapi.c or any of its callers.
 */

namespace {

struct OpusNum {
    union {
        char bytes[8];
        double value;
    } num;
};

struct OpusUnpackedString {
    char digits[15];
    char count;
    std::uint32_t exponent_and_sign;
};

static_assert(sizeof(OpusNum) == 8);

double accumulator = 0.0;
int math_error = 0;

constexpr int kMathOverflow = 0x0001;
constexpr int kMathUnderflow = 0x0002;
constexpr int kMathDivideByZero = 0x0004;
constexpr int kMathTranscendental = 0x0008;

void record_result(const double result) noexcept {
    accumulator = result;
    if (std::isinf(result)) {
        math_error |= kMathOverflow;
    } else if (std::isnan(result)) {
        math_error |= kMathTranscendental;
    } else if (result != 0.0 && std::fpclassify(result) == FP_SUBNORMAL) {
        math_error |= kMathUnderflow;
    }
}

}  // namespace

extern "C" {

/* The original diagnostic reports whether the optional 8087 path is active.
 * AMD64 always has hardware floating-point support. */
int f8087 = 1;
int fError = 0;

int InitMathPack() {
    accumulator = 0.0;
    math_error = 0;
    fError = 0;
    return 1;
}

int FInitMathPack() { return InitMathPack(); }
void TermMathPack() {}

int DError(const int replacement) {
    const int previous = math_error;
    math_error = replacement;
    fError = replacement != 0;
    return previous;
}

void Dld(const OpusNum* number) {
    accumulator = number == nullptr ? 0.0 : number->num.value;
}

void Dst(OpusNum* number) {
    if (number != nullptr) {
        number->num.value = accumulator;
    }
}

void DClr() { accumulator = 0.0; }

void DLdC(const int index) {
    static constexpr double constants[] = {
        1.0, 2.0, -1.0, 10.0, 100.0,
        2.30258509299404568402, 0.5, -2.0e60
    };
    if (index < 0 || index >= static_cast<int>(sizeof(constants) /
                                                sizeof(constants[0]))) {
        accumulator = 0.0;
        math_error |= kMathTranscendental;
        return;
    }
    accumulator = constants[index];
}

void DFloat(const unsigned int value) {
    accumulator = static_cast<double>(value);
}

void DAdd(const OpusNum* operand) {
    record_result(accumulator + operand->num.value);
}

void DSub(const OpusNum* operand) {
    record_result(accumulator - operand->num.value);
}

void DMul(const OpusNum* operand) {
    record_result(accumulator * operand->num.value);
}

void DDiv(const OpusNum* operand) {
    if (operand->num.value == 0.0) {
        math_error |= kMathDivideByZero;
        record_result(std::copysign((std::numeric_limits<double>::infinity)(),
                                    accumulator));
        return;
    }
    record_result(accumulator / operand->num.value);
}

void DNeg() { accumulator = -accumulator; }

int DCond() {
    if (std::isnan(accumulator)) {
        math_error |= kMathTranscendental;
        return 0;
    }
    return accumulator < 0.0 ? -1 : accumulator > 0.0 ? 1 : 0;
}

void DInt() { accumulator = std::trunc(accumulator); }

unsigned int Fix() {
    const double fixed = std::trunc(accumulator);
    if (!std::isfinite(fixed) || fixed < 0.0 || fixed > 65535.0) {
        math_error |= kMathOverflow;
        return fixed < 0.0 ? 0u : 65535u;
    }
    return static_cast<unsigned int>(fixed);
}

void TenTo(const int exponent) {
    record_result(std::pow(10.0, static_cast<double>(exponent)));
}

void Exp() { record_result(std::exp(accumulator)); }

void Ln() {
    if (accumulator <= 0.0) {
        math_error |= kMathTranscendental;
        accumulator = (std::numeric_limits<double>::quiet_NaN)();
        return;
    }
    record_result(std::log(accumulator));
}

void Sqr() {
    if (accumulator < 0.0) {
        math_error |= kMathTranscendental;
        accumulator = (std::numeric_limits<double>::quiet_NaN)();
        return;
    }
    record_result(std::sqrt(accumulator));
}

void SQR() { Sqr(); }

void Unpack(OpusUnpackedString* unpacked) {
    if (unpacked == nullptr) {
        return;
    }
    std::memset(unpacked, 0, sizeof(*unpacked));

    const bool negative = std::signbit(accumulator);
    const double magnitude = std::fabs(accumulator);
    if (magnitude == 0.0 || !std::isfinite(magnitude)) {
        unpacked->digits[0] = '0';
        unpacked->count = 1;
        unpacked->exponent_and_sign =
            0x4000u | (negative ? 0x8000u : 0u);
        if (!std::isfinite(magnitude)) {
            math_error |= kMathTranscendental;
        }
        return;
    }

    char scientific[32]{};
    std::snprintf(scientific, sizeof(scientific), "%.14e", magnitude);
    const char* exponent_marker = std::strchr(scientific, 'e');
    const int decimal_exponent =
        exponent_marker == nullptr ? 0 : std::atoi(exponent_marker + 1);

    int count = 0;
    for (const char* cursor = scientific;
         cursor != exponent_marker && *cursor != '\0' && count < 15;
         ++cursor) {
        if (*cursor >= '0' && *cursor <= '9') {
            unpacked->digits[count++] = *cursor;
        }
    }
    while (count > 1 && unpacked->digits[count - 1] == '0') {
        --count;
    }
    unpacked->count = static_cast<char>(count);
    unpacked->exponent_and_sign =
        static_cast<std::uint32_t>(0x4000 + decimal_exponent + 1) |
        (negative ? 0x8000u : 0u);
}

/* token.c contains the historical mixed-case spelling. */
int CNumInt(int value);
int CnumInt(const int value) { return CNumInt(value); }

}  // extern "C"
