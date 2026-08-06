#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace opus::x64 {

// Fixed-width replacements for assembly primitives that are still useful to
// the native port. Higher-level Opus routines are replaced by the portable C
// algorithms in Opus/wordtech as that document engine is migrated.

struct LegacyTime {
    std::uint16_t hour{};
    std::uint16_t minute{};
    std::uint16_t second{};
    std::uint16_t hundredths{};
};

struct LegacyDate {
    std::uint16_t year{};
    std::uint16_t month{};
    std::uint16_t day{};
    std::uint16_t day_of_week{};
};

[[nodiscard]] std::int32_t multiply_divide(std::int32_t value,
                                            std::int32_t numerator,
                                            std::int32_t denominator) noexcept;
[[nodiscard]] std::uint32_t unsigned_multiply_divide(std::uint32_t value,
                                                      std::uint32_t numerator,
                                                      std::uint32_t denominator) noexcept;
[[nodiscard]] double legacy_log(double value) noexcept;
[[nodiscard]] double legacy_exp(double value) noexcept;
[[nodiscard]] double legacy_sqrt(double value) noexcept;
[[nodiscard]] double legacy_square(double value) noexcept;

void fill_byte_pattern(void* destination, std::size_t byte_count,
                       const void* pattern, std::size_t pattern_size);
[[nodiscard]] bool buffers_differ(const void* left, const void* right,
                                  std::size_t byte_count) noexcept;
[[nodiscard]] std::size_t zero_terminated_length(const char* text) noexcept;
[[nodiscard]] std::size_t copy_zero_terminated(char* destination,
                                                std::size_t destination_size,
                                                const char* source) noexcept;
[[nodiscard]] bool is_ascii_alpha(std::uint8_t value) noexcept;
[[nodiscard]] bool is_ascii_digit(std::uint8_t value) noexcept;
[[nodiscard]] bool is_ascii_alphanumeric(std::uint8_t value) noexcept;
[[nodiscard]] std::uint8_t ascii_upper(std::uint8_t value) noexcept;

[[nodiscard]] LegacyTime local_time() noexcept;
[[nodiscard]] LegacyDate local_date() noexcept;
[[nodiscard]] std::uint64_t available_disk_bytes(std::wstring_view path) noexcept;
[[nodiscard]] bool is_remote_path(std::wstring_view path) noexcept;

// Returns true only if the checked-in migration table accounts for every
// legacy .asm module. This is also run during application startup.
[[nodiscard]] bool assembly_manifest_complete() noexcept;
[[nodiscard]] std::size_t assembly_manifest_count() noexcept;

}  // namespace opus::x64

