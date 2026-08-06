#include "legacy_asm_port.h"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace opus::x64 {
namespace {

enum class PortKind : std::uint8_t {
    translated_cpp,
    portable_c_counterpart,
    win32_replacement,
    retired_platform_code,
};

struct AssemblyPortEntry {
    std::string_view path;
    PortKind kind;
};

// One entry per .asm file under src. This table is intentionally explicit:
// adding an assembly module makes CMake fail until its x64 disposition is
// reviewed. "portable_c_counterpart" means the hand-written assembly was an
// optimized version of an extant C implementation in Opus/wordtech.
constexpr std::array kAssemblyPorts{
    AssemblyPortEntry{"Opus/asm/asserth.asm", PortKind::retired_platform_code},
    AssemblyPortEntry{"Opus/asm/auxout.asm", PortKind::win32_replacement},
    AssemblyPortEntry{"Opus/asm/clsplcn.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/cmdlookn.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/dialog1n.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/disp1n.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/disp1n2.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/disp2n.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/disp3n.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/disptbln.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/disptbn2.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/editn.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/editn2.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/eldden.asm", PortKind::win32_replacement},
    AssemblyPortEntry{"Opus/asm/expn.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/fetch2n.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/fetchn.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/fetchn2.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/fetchn3.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/fetchtbn.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/fieldcrn.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/fieldfmn.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/fieldspn.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/file2n.asm", PortKind::win32_replacement},
    AssemblyPortEntry{"Opus/asm/filewinn.asm", PortKind::win32_replacement},
    AssemblyPortEntry{"Opus/asm/formatn.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/formatn2.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/formulan.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/grbitx.asm", PortKind::translated_cpp},
    AssemblyPortEntry{"Opus/asm/grhc.asm", PortKind::translated_cpp},
    AssemblyPortEntry{"Opus/asm/index2n.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/inssubsn.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/int3f.asm", PortKind::retired_platform_code},
    AssemblyPortEntry{"Opus/asm/layout22.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/layout2n.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/movecmds.asm", PortKind::retired_platform_code},
    AssemblyPortEntry{"Opus/asm/natinit.asm", PortKind::translated_cpp},
    AssemblyPortEntry{"Opus/asm/nattrans.asm", PortKind::translated_cpp},
    AssemblyPortEntry{"Opus/asm/prln.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/profc.asm", PortKind::retired_platform_code},
    AssemblyPortEntry{"Opus/asm/profctl.asm", PortKind::retired_platform_code},
    AssemblyPortEntry{"Opus/asm/promptn.asm", PortKind::win32_replacement},
    AssemblyPortEntry{"Opus/asm/qplc.asm", PortKind::retired_platform_code},
    AssemblyPortEntry{"Opus/asm/resn.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/resn2.asm", PortKind::translated_cpp},
    AssemblyPortEntry{"Opus/asm/rip.asm", PortKind::retired_platform_code},
    AssemblyPortEntry{"Opus/asm/rtfinn.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/rtfsubsn.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/searchn.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/sttbn.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/wordgrep.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"Opus/asm/wprocn.asm", PortKind::win32_replacement},
    AssemblyPortEntry{"OpusEtAl/tools/src/appstub.asm", PortKind::retired_platform_code},
    AssemblyPortEntry{"OpusEtAl/tools/src/convtest/doslib.asm", PortKind::win32_replacement},
    AssemblyPortEntry{"OpusEtAl/tools/src/convtest/init.asm", PortKind::retired_platform_code},
    AssemblyPortEntry{"OpusEtAl/tools/src/makekeyn.asm", PortKind::portable_c_counterpart},
    AssemblyPortEntry{"OpusEtAl/tools/src/timedate.asm", PortKind::translated_cpp},
    AssemblyPortEntry{"OpusEtAl/tools/src/winemscs.asm", PortKind::retired_platform_code},
    AssemblyPortEntry{"OpusEtAl/tools/src/winemsds.asm", PortKind::retired_platform_code},
};

static_assert(kAssemblyPorts.size() == 59);

}  // namespace

std::int32_t multiply_divide(const std::int32_t value,
                             const std::int32_t numerator,
                             const std::int32_t denominator) noexcept {
    if (denominator == 0) {
        const bool negative = (value < 0) != (numerator < 0);
        return negative ? std::numeric_limits<std::int32_t>::min()
                        : std::numeric_limits<std::int32_t>::max();
    }
    const auto result = (static_cast<std::int64_t>(value) * numerator) / denominator;
    return static_cast<std::int32_t>(std::clamp<std::int64_t>(
        result, std::numeric_limits<std::int32_t>::min(),
        std::numeric_limits<std::int32_t>::max()));
}

std::uint32_t unsigned_multiply_divide(const std::uint32_t value,
                                       const std::uint32_t numerator,
                                       const std::uint32_t denominator) noexcept {
    if (denominator == 0) {
        return std::numeric_limits<std::uint32_t>::max();
    }
    const auto result = (static_cast<std::uint64_t>(value) * numerator) / denominator;
    return static_cast<std::uint32_t>(std::min<std::uint64_t>(
        result, std::numeric_limits<std::uint32_t>::max()));
}

double legacy_log(const double value) noexcept { return std::log(value); }
double legacy_exp(const double value) noexcept { return std::exp(value); }
double legacy_sqrt(const double value) noexcept { return std::sqrt(value); }
double legacy_square(const double value) noexcept { return value * value; }

void fill_byte_pattern(void* destination, const std::size_t byte_count,
                       const void* pattern, const std::size_t pattern_size) {
    if (byte_count == 0) {
        return;
    }
    if (destination == nullptr || pattern == nullptr || pattern_size == 0) {
        throw std::invalid_argument("fill_byte_pattern received an invalid buffer");
    }
    auto* output = static_cast<std::byte*>(destination);
    const auto* input = static_cast<const std::byte*>(pattern);
    for (std::size_t offset = 0; offset < byte_count; ++offset) {
        output[offset] = input[offset % pattern_size];
    }
}

bool buffers_differ(const void* left, const void* right,
                    const std::size_t byte_count) noexcept {
    if (byte_count == 0) {
        return false;
    }
    if (left == nullptr || right == nullptr) {
        return true;
    }
    return std::memcmp(left, right, byte_count) != 0;
}

std::size_t zero_terminated_length(const char* text) noexcept {
    return text == nullptr ? 0 : std::strlen(text);
}

std::size_t copy_zero_terminated(char* destination,
                                 const std::size_t destination_size,
                                 const char* source) noexcept {
    if (destination == nullptr || destination_size == 0) {
        return 0;
    }
    if (source == nullptr) {
        destination[0] = '\0';
        return 0;
    }
    const auto count = std::min(destination_size - 1, std::strlen(source));
    std::memcpy(destination, source, count);
    destination[count] = '\0';
    return count;
}

bool is_ascii_alpha(const std::uint8_t value) noexcept {
    const auto upper = ascii_upper(value);
    return upper >= 'A' && upper <= 'Z';
}

bool is_ascii_digit(const std::uint8_t value) noexcept {
    return value >= '0' && value <= '9';
}

bool is_ascii_alphanumeric(const std::uint8_t value) noexcept {
    return is_ascii_alpha(value) || is_ascii_digit(value);
}

std::uint8_t ascii_upper(const std::uint8_t value) noexcept {
    return value >= 'a' && value <= 'z' ? static_cast<std::uint8_t>(value - ('a' - 'A'))
                                        : value;
}

LegacyTime local_time() noexcept {
    SYSTEMTIME value{};
    GetLocalTime(&value);
    return {value.wHour, value.wMinute, value.wSecond,
            static_cast<std::uint16_t>(value.wMilliseconds / 10)};
}

LegacyDate local_date() noexcept {
    SYSTEMTIME value{};
    GetLocalTime(&value);
    return {value.wYear, value.wMonth, value.wDay, value.wDayOfWeek};
}

std::uint64_t available_disk_bytes(const std::wstring_view path) noexcept {
    ULARGE_INTEGER available{};
    const std::wstring owned_path(path);
    if (!GetDiskFreeSpaceExW(owned_path.c_str(), &available, nullptr, nullptr)) {
        return 0;
    }
    return available.QuadPart;
}

bool is_remote_path(const std::wstring_view path) noexcept {
    if (path.starts_with(L"\\\\")) {
        return true;
    }
    if (path.size() < 2 || path[1] != L':') {
        return false;
    }
    wchar_t root[] = {path[0], L':', L'\\', L'\0'};
    return GetDriveTypeW(root) == DRIVE_REMOTE;
}

bool assembly_manifest_complete() noexcept {
    for (std::size_t left = 0; left < kAssemblyPorts.size(); ++left) {
        if (kAssemblyPorts[left].path.empty()) {
            return false;
        }
        for (std::size_t right = left + 1; right < kAssemblyPorts.size(); ++right) {
            if (kAssemblyPorts[left].path == kAssemblyPorts[right].path) {
                return false;
            }
        }
    }
    return true;
}

std::size_t assembly_manifest_count() noexcept { return kAssemblyPorts.size(); }

}  // namespace opus::x64
