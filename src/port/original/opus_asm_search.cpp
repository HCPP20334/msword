#include "opus_x64_compat.h"

#include <algorithm>
#include <cstdint>
#include <vector>

/* AMD64 translation boundary for SEARCHN.ASM and WORDGREP.ASM.
 *
 * Microsoft's portable search implementation is retained in search.c.  The
 * first four exports replace the assembly-only release entry points with
 * calls to that original C implementation.  Wordgrep is translated below as
 * a streaming regular-expression matcher over the original DOS file handle.
 */

namespace {

enum class GrepAtomKind : std::uint8_t {
    literal,
    any,
    non_space,
};

struct GrepAtom {
    GrepAtomKind kind;
    unsigned char value;
    bool repeat;
};

unsigned char fold_ansi(unsigned char value) noexcept {
    char byte = static_cast<char>(value);
    CharUpperBuffA(&byte, 1);
    return static_cast<unsigned char>(byte);
}

std::vector<GrepAtom> compile_pattern(const char* pattern) {
    std::vector<GrepAtom> atoms;
    if (pattern == nullptr) {
        return atoms;
    }

    const auto* current = reinterpret_cast<const unsigned char*>(pattern);
    while (*current != 0) {
        bool escaped = false;
        unsigned char value = *current++;
        if (value == '\\' && *current != 0) {
            escaped = true;
            value = *current++;
        }

        if (!escaped && value == '*' && !atoms.empty()) {
            atoms.back().repeat = true;
            continue;
        }

        GrepAtom atom{GrepAtomKind::literal, value, false};
        if (!escaped && value == '.') {
            atom.kind = GrepAtomKind::any;
        } else if (!escaped && value == '#') {
            atom.kind = GrepAtomKind::non_space;
        }
        atoms.push_back(atom);
    }
    return atoms;
}

bool atom_matches(const GrepAtom& atom, const unsigned char byte,
                  const bool ignore_case) noexcept {
    switch (atom.kind) {
    case GrepAtomKind::any:
        return true;
    case GrepAtomKind::non_space:
        return byte != 0 && byte != '\t' && byte != '\n' && byte != '\r' &&
               byte != ' ';
    case GrepAtomKind::literal:
        return ignore_case ? fold_ansi(byte) == fold_ansi(atom.value)
                           : byte == atom.value;
    }
    return false;
}

void add_epsilon_closure(const std::vector<GrepAtom>& atoms,
                         std::vector<unsigned char>& states) {
    for (std::size_t index = 0; index < atoms.size(); ++index) {
        if (states[index] != 0 && atoms[index].repeat) {
            states[index + 1] = 1;
        }
    }
}

class StreamingGrep {
public:
    StreamingGrep(const char* pattern, const bool ignore_case)
        : atoms_(compile_pattern(pattern)),
          states_(atoms_.size() + 1),
          ignore_case_(ignore_case),
          matched_(atoms_.empty()) {}

    void consume(const unsigned char byte) {
        if (matched_) {
            return;
        }

        states_[0] = 1; /* A match may begin at every byte. */
        add_epsilon_closure(atoms_, states_);

        std::vector<unsigned char> next(atoms_.size() + 1);
        for (std::size_t index = 0; index < atoms_.size(); ++index) {
            if (states_[index] == 0 ||
                !atom_matches(atoms_[index], byte, ignore_case_)) {
                continue;
            }
            next[atoms_[index].repeat ? index : index + 1] = 1;
        }
        add_epsilon_closure(atoms_, next);
        states_.swap(next);
        matched_ = states_.back() != 0;
    }

    bool matched() const noexcept { return matched_; }

private:
    std::vector<GrepAtom> atoms_;
    std::vector<unsigned char> states_;
    bool ignore_case_;
    bool matched_;
};

}  // namespace

extern "C" {

std::int32_t C_CpSearchSz(void* bmib, std::int32_t first,
                          std::int32_t limit, std::int32_t* next_report,
                          void* progress_handle);
std::int32_t C_CpSearchSzBackward(void* bmib, std::int32_t first,
                                  std::int32_t limit);
int C_FMatchChp();
int C_FMatchPap();
int C_WCompRgchIndex(char* first, int first_count, char* second,
                     int second_count);

int CchReadDoshnd(HFILE file, void* buffer, unsigned int bytes);
long DwSeekDw(HFILE file, long offset, int origin);

std::int32_t N_CpSearchSz(void* bmib, const std::int32_t first,
                          const std::int32_t limit,
                          std::int32_t* const next_report,
                          void* const progress_handle) {
    return C_CpSearchSz(bmib, first, limit, next_report, progress_handle);
}

std::int32_t N_CpSearchSzBackward(void* bmib, const std::int32_t first,
                                  const std::int32_t limit) {
    return C_CpSearchSzBackward(bmib, first, limit);
}

int FMatchChp() { return C_FMatchChp(); }
int FMatchPap() { return C_FMatchPap(); }

int N_WCompRgchIndex(char* first, const int first_count, char* second,
                     const int second_count) {
    return C_WCompRgchIndex(first, first_count, second, second_count);
}

int wordgrep(void* const memory, const int memory_bytes,
             const char* const* const strings, const int string_count,
             unsigned char* const results, const HFILE file,
             const int ignore_case, const std::int32_t first,
             const std::int32_t byte_count) {
    if (strings == nullptr || results == nullptr || string_count <= 0) {
        return 0;
    }

    std::vector<StreamingGrep> matchers;
    matchers.reserve(static_cast<std::size_t>(string_count));
    for (int index = 0; index < string_count; ++index) {
        matchers.emplace_back(strings[index], ignore_case != 0);
        if (matchers.back().matched()) {
            results[index] = 1;
        }
    }

    if (memory == nullptr || memory_bytes <= 0 || byte_count <= 0 ||
        DwSeekDw(file, first, FILE_BEGIN) < 0) {
        return 0;
    }

    auto* const buffer = static_cast<unsigned char*>(memory);
    std::int32_t remaining = byte_count;
    while (remaining > 0) {
        const auto request = static_cast<unsigned int>(std::min<std::int32_t>(
            remaining, static_cast<std::int32_t>(memory_bytes)));
        const int read = CchReadDoshnd(file, buffer, request);
        if (read <= 0) {
            break;
        }

        for (int byte_index = 0; byte_index < read; ++byte_index) {
            for (int pattern_index = 0; pattern_index < string_count;
                 ++pattern_index) {
                if (results[pattern_index] == 0) {
                    matchers[pattern_index].consume(buffer[byte_index]);
                    if (matchers[pattern_index].matched()) {
                        results[pattern_index] = 1;
                    }
                }
            }
        }

        remaining -= read;
        if (read < static_cast<int>(request) ||
            std::all_of(matchers.begin(), matchers.end(),
                        [](const StreamingGrep& matcher) {
                            return matcher.matched();
                        })) {
            break;
        }
    }
    return 0;
}

}  // extern "C"
