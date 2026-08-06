#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

/* AMD64 translations of the active GRBITX.ASM and GRHC.ASM entry points. */
extern "C" {

int SlowStrBltRow(void* picture, unsigned char* source,
                  unsigned char* destination);
void C_GetMonoStrBltInfo(void* picture, unsigned int* values);
int C_ReadTIFFLine(void** picture_handle, unsigned char* plane);

void StrBltRow(void* picture, unsigned char* source,
               unsigned char* destination) {
    (void)SlowStrBltRow(picture, source, destination);
}

void MonoStrBltRow(void* picture, const unsigned char* source,
                   unsigned char* destination, const unsigned int row,
                   const unsigned char* patterns) {
    if (picture == nullptr || source == nullptr || destination == nullptr ||
        patterns == nullptr) {
        return;
    }

    unsigned int values[7]{};
    C_GetMonoStrBltInfo(picture, values);
    const unsigned int input_planes = values[0];
    const unsigned int output_planes = values[1];
    const std::size_t plane_bytes = values[2];
    const unsigned int source_width = values[3];
    const unsigned int destination_width = values[4];
    const unsigned int normal_repeat = values[5];
    const unsigned int repeat_delta = values[6];

    if (input_planes == 0 || input_planes > 4 || output_planes != 1 ||
        source_width == 0) {
        return;
    }

    const std::size_t destination_bytes =
        (static_cast<std::size_t>(destination_width) + 7u) / 8u;
    std::memset(destination, 0, destination_bytes);

    const unsigned char* pattern_row = patterns + ((row & 7u) * 16u);
    unsigned int remainder = 0;
    unsigned int destination_bit = 0;
    for (unsigned int source_bit = 0;
         source_bit < source_width && destination_bit < destination_width;
         ++source_bit) {
        remainder += repeat_delta;
        unsigned int repeat = normal_repeat;
        if (remainder >= source_width) {
            ++repeat;
            remainder -= source_width;
        }

        unsigned int color = 0;
        for (unsigned int plane = input_planes; plane-- > 0;) {
            const unsigned char byte =
                source[static_cast<std::size_t>(plane) * plane_bytes +
                       source_bit / 8u];
            color = (color << 1u) |
                    ((byte >> (7u - (source_bit & 7u))) & 1u);
        }

        const unsigned char pattern = pattern_row[color & 0x0fu];
        while (repeat-- > 0 && destination_bit < destination_width) {
            const unsigned int bit_in_byte = destination_bit & 7u;
            if (((pattern >> (7u - bit_in_byte)) & 1u) != 0) {
                destination[destination_bit / 8u] |=
                    static_cast<unsigned char>(0x80u >> bit_in_byte);
            }
            ++destination_bit;
        }
    }
}

void ReadTIFFLineNat(void** picture_handle, unsigned char* plane) {
    (void)C_ReadTIFFLine(picture_handle, plane);
}

unsigned int UDiv(const unsigned int numerator,
                  const unsigned int denominator,
                  unsigned int* remainder) {
    if (denominator == 0) {
        if (remainder != nullptr) {
            *remainder = 0;
        }
        return 0xffffu;
    }
    if (remainder != nullptr) {
        *remainder = numerator % denominator;
    }
    return numerator / denominator;
}

}  // extern "C"
