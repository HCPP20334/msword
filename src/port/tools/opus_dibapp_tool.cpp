#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace
{
std::uint16_t ReadU16(const std::vector<unsigned char>& bytes,
		std::size_t offset)
	{
	return static_cast<std::uint16_t>(bytes[offset]) |
			(static_cast<std::uint16_t>(bytes[offset + 1]) << 8);
	}

std::uint32_t ReadU32(const std::vector<unsigned char>& bytes,
		std::size_t offset)
	{
	return static_cast<std::uint32_t>(ReadU16(bytes, offset)) |
			(static_cast<std::uint32_t>(ReadU16(bytes, offset + 2)) << 16);
	}
}

int main(int argc, char **argv)
	{
	if (argc != 3)
		{
		std::cerr << "usage: opus_dibapp_tool input.dib output.hb\n";
		return 2;
		}

	std::ifstream input(argv[1], std::ios::binary);
	if (!input)
		{
		std::cerr << "cannot open input: " << argv[1] << '\n';
		return 1;
		}
	std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input)),
			std::istreambuf_iterator<char>());

	/* The archived files are OS/2 bitmap-array files: a 14-byte BA record,
	 * a 14-byte BM record, a 12-byte BITMAPCOREHEADER, a 3-byte-per-color
	 * palette, then DWORD-aligned scan lines. */
	if (bytes.size() < 46 || bytes[0] != 'B' || bytes[1] != 'A' ||
			bytes[14] != 'B' || bytes[15] != 'M' || ReadU32(bytes, 28) != 12)
		{
		std::cerr << "unsupported archived DIB layout: " << argv[1] << '\n';
		return 1;
		}

	const std::uint16_t width = ReadU16(bytes, 32);
	const std::uint16_t height = ReadU16(bytes, 34);
	const std::uint16_t planes = ReadU16(bytes, 36);
	const std::uint16_t bit_count = ReadU16(bytes, 38);
	if (planes != 1 || (bit_count != 1 && bit_count != 4))
		{
		std::cerr << "unsupported DIB bit depth: " << argv[1] << '\n';
		return 1;
		}

	const std::size_t palette_bytes =
			(static_cast<std::size_t>(1) << bit_count) * 3;
	const std::size_t bits_offset = 40 + palette_bytes;
	const std::size_t row_bytes =
			((static_cast<std::size_t>(width) * bit_count + 31) / 32) * 4;
	const std::size_t bits_size = row_bytes * height;
	if (bits_offset + bits_size != bytes.size())
		{
		std::cerr << "DIB size does not match its core header: " << argv[1]
				<< '\n';
		return 1;
		}

	std::ofstream output(argv[2], std::ios::binary | std::ios::trunc);
	if (!output)
		{
		std::cerr << "cannot open output: " << argv[2] << '\n';
		return 1;
		}

	output << "/* THIS IS A GENERATED FILE -- DO NOT EDIT */\n\n"
			<< "{\n\t{ 12, " << width << ", " << height << ", "
			<< planes << ", " << bit_count << " },\n\t{\n";
	output << std::hex << std::setfill('0');
	for (std::size_t index = 0; index < bits_size; ++index)
		{
		if (index % 12 == 0)
			output << "\t\t";
		output << "0x" << std::setw(2)
				<< static_cast<unsigned>(bytes[bits_offset + index]);
		if (index + 1 != bits_size)
			output << ", ";
		if (index % 12 == 11 || index + 1 == bits_size)
			output << '\n';
		}
	output << std::dec << "\t},\n\t" << bits_size << ",\n},\n";

	return output ? 0 : 1;
	}

