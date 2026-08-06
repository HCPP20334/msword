#include "opus_x64_compat.h"
#include "inter.h"

#include <cstddef>
#include <cstring>

extern "C" struct ITR vitr;

namespace {

unsigned char french_sort_upper(const unsigned char character) noexcept {
    switch (character) {
    case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5:
    case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5:
        return 'A';
    case 0xC7: case 0xE7:
        return 'C';
    case 0xC8: case 0xC9: case 0xCA: case 0xCB:
    case 0xE8: case 0xE9: case 0xEA: case 0xEB:
        return 'E';
    case 0xCC: case 0xCD: case 0xCE: case 0xCF:
    case 0xEC: case 0xED: case 0xEE: case 0xEF:
        return 'I';
    case 0xD1: case 0xF1:
        return 'N';
    case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6:
    case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF6:
        return 'O';
    case 0xD9: case 0xDA: case 0xDB: case 0xDC:
    case 0xF9: case 0xFA: case 0xFB: case 0xFC:
        return 'U';
    case 0xDD: case 0xFD: case 0xFF:
        return 'Y';
    default:
        return character;
    }
}

unsigned char native_upper(unsigned char character) noexcept {
    if (vitr.fFrench) {
        character = french_sort_upper(character);
    }
    char text = static_cast<char>(character);
    CharUpperBuffA(&text, 1);
    return static_cast<unsigned char>(text);
}

WORD character_type(const int character) noexcept {
    char text = static_cast<char>(static_cast<unsigned char>(character));
    WORD type = 0;
    GetStringTypeA(LOCALE_USER_DEFAULT, CT_CTYPE1, &text, 1, &type);
    return type;
}

}  // namespace

/* Native utility translations from the active tail of RESN2.ASM. */
extern "C" {

int CchSz(const CHAR* text) {
    return text == nullptr ? 0 : static_cast<int>(std::strlen(text) + 1);
}

int CchCopySz(const CHAR* source, CHAR* destination) {
    const std::size_t count = std::strlen(source);
    std::memmove(destination, source, count + 1);
    return static_cast<int>(count);
}

CHAR* PchInSt(CHAR* counted_string, const int character) {
    if (counted_string == nullptr) {
        return nullptr;
    }
    const unsigned int count =
        static_cast<unsigned char>(counted_string[0]);
    for (unsigned int index = 1; index <= count; ++index) {
        if (static_cast<unsigned char>(counted_string[index]) ==
            static_cast<unsigned char>(character)) {
            return counted_string + index;
        }
    }
    return nullptr;
}

int FAlphaNum(const int character) {
    return (character_type(character) & (C1_ALPHA | C1_DIGIT)) != 0;
}

int FDigit(const int character) {
    return (character_type(character) & C1_DIGIT) != 0;
}

int FAlpha(const int character) {
    return (character_type(character) & C1_ALPHA) != 0;
}

int FUpper(const int character) {
    return (character_type(character) & C1_UPPER) != 0;
}

int FLower(const int character) {
    return (character_type(character) & C1_LOWER) != 0;
}

CHAR ChUpper(const int character) {
    return static_cast<CHAR>(native_upper(
        static_cast<unsigned char>(character)));
}

void QszUpper(CHAR* text) {
    if (text == nullptr) {
        return;
    }
    while (*text != '\0') {
        *text = ChUpper(static_cast<unsigned char>(*text));
        ++text;
    }
}

int FNeNcRgch(const CHAR* first, const CHAR* second, const int count) {
    for (int index = 0; index < count; ++index) {
        if (ChUpper(static_cast<unsigned char>(first[index])) !=
            ChUpper(static_cast<unsigned char>(second[index]))) {
            return 1;
        }
    }
    return 0;
}

int FNeNcSz(const CHAR* first, const CHAR* second) {
    for (;;) {
        const unsigned char left = static_cast<unsigned char>(*first++);
        const unsigned char right = static_cast<unsigned char>(*second++);
        if (ChUpper(left) != ChUpper(right)) {
            return 1;
        }
        if (left == 0 || right == 0) {
            return left != right;
        }
    }
}

int FNeNcSt(const CHAR* first, const CHAR* second) {
    const int first_count = static_cast<unsigned char>(first[0]);
    const int second_count = static_cast<unsigned char>(second[0]);
    return first_count != second_count ||
           FNeNcRgch(first + 1, second + 1, first_count);
}

int FSetRgchDiff(const char* base, const char* updated, char* differences,
                 unsigned int count) {
    unsigned char any_difference = 0;
    while (count-- != 0) {
        const unsigned char difference =
            static_cast<unsigned char>(*base++ ^ *updated++);
        *differences = static_cast<char>(
            static_cast<unsigned char>(*differences) | difference);
        ++differences;
        any_difference = static_cast<unsigned char>(any_difference |
                                                    difference);
    }
    return any_difference != 0;
}

int C_FSetRgchDiff(const char* base, const char* updated, char* differences,
                   const unsigned int count) {
    return FSetRgchDiff(base, updated, differences, count);
}

int C_FSetRgwDiff(const int* base, const int* updated, int* differences,
                  unsigned int count) {
    int any_difference = 0;
    while (count-- != 0) {
        const int difference = *base++ ^ *updated++;
        *differences++ |= difference;
        any_difference |= difference;
    }
    return any_difference != 0;
}

int IScanLprgw(const int* values, const int wanted, const int count) {
    for (int index = 0; index < count; ++index) {
        if (values[index] == wanted) {
            return index;
        }
    }
    return -1;
}

int CchCopyLpszCchMax(const char* source, char* destination,
                      const int capacity) {
    if (destination == nullptr || capacity <= 0) {
        return 0;
    }
    int count = 0;
    while (count < capacity &&
           (destination[count] = source[count]) != '\0') {
        ++count;
    }
    if (count == capacity) {
        destination[capacity - 1] = '\0';
    }
    return count;
}

}  // extern "C"
