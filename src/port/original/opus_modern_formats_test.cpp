#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <climits>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

extern "C" int OpusModernDocxToRtfFile(const char*, const char*);
extern "C" int OpusModernDocxToTextFile(const char*, const char*);
extern "C" int OpusModernRtfFileToDocx(const char*, const char*);
extern "C" int OpusModernRtfFileToPdf(const char*, const char*);
extern "C" int OpusModernBindPendingDocxUnicode(int);
extern "C" unsigned int OpusUnicodeScalarAt(int, long);
extern "C" int OpusPdfSnapshotBegin(int, int, int, int, int, int);
extern "C" int OpusPdfSnapshotAddParagraph(int, int, int, int, int, int,
                                            int, int, int, int, int);
extern "C" int OpusPdfSnapshotAddRun(const char*, int, const char*, int,
                                      int, int, int, int, int, int, int, int);

std::string read_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), {}};
}

int main(const int argument_count, char** arguments) {
    if (argument_count == 4 && std::strcmp(arguments[1], "--text") == 0) {
        const bool converted =
            OpusModernDocxToTextFile(arguments[2], arguments[3]) != 0;
        std::cout << "DOCX text import "
                  << (converted ? "passed" : "failed") << '\n';
        return converted ? 0 : 4;
    }
    if (argument_count == 3) {
        const bool converted =
            OpusModernDocxToRtfFile(arguments[1], arguments[2]) != 0;
        std::cout << "DOCX import " << (converted ? "passed" : "failed")
                  << '\n';
        return converted ? 0 : 3;
    }
    if (argument_count != 1) {
        std::cerr << "usage: opus_modern_formats_test [input.docx output.rtf]\n";
        return 1;
    }
    char temporary[MAX_PATH]{};
    char seed[MAX_PATH]{};
    if (GetTempPathA(MAX_PATH, temporary) == 0 ||
        GetTempFileNameA(temporary, "OWF", 0, seed) == 0) {
        return 1;
    }
    DeleteFileA(seed);
    const std::string base = seed;
    const std::string rtf = base + ".rtf";
    const std::string docx = base + ".docx";
    const std::string roundtrip = base + ".roundtrip.rtf";
    const std::string imported_text = base + ".unicode.txt";
    const std::string oversized = base + ".oversized.rtf";
    const std::string preserved = base + ".preserved.pdf";
    char requested_pdf[32768]{};
    const DWORD requested_pdf_length = GetEnvironmentVariableA(
        "WORD1_TEST_KEEP_PDF", requested_pdf,
        static_cast<DWORD>(std::size(requested_pdf)));
    const bool keep_pdf = requested_pdf_length > 0 &&
                          requested_pdf_length < std::size(requested_pdf);
    const std::string pdf = keep_pdf ? requested_pdf : base + ".pdf";
    DeleteFileA(pdf.c_str());
    {
        std::ofstream output(rtf, std::ios::binary);
        output << "{\\rtf1\\ansi{\\fonttbl{\\f0 Arial;}}"
                  "{\\colortbl;\\red255\\green0\\blue0;}"
                  "\\f0\\fs24 Plain {\\b Bold} {\\i Italic} "
                  "{\\ul Underline} {\\cf1 Red} "
                  "{\\lang1049 \\u1055?\\u1088?\\u1080?\\u1074?\\u1077?\\u1090?}"
                  " {\\lang1032 \\u915?\\u949?\\u953?\\u940?}"
                  " {\\lang1025 \\u1605?\\u1585?\\u1581?\\u1576?\\u1575?}"
                  " {\\lang1041 \\u12371?\\u12435?\\u12395?\\u12385?\\u12399?}"
                  "\\par Second paragraph}";
    }
    const bool written = OpusModernRtfFileToDocx(rtf.c_str(), docx.c_str()) != 0;
    const bool read = written &&
        OpusModernDocxToRtfFile(docx.c_str(), roundtrip.c_str()) != 0;
    const bool unicode_imported = written &&
        OpusModernDocxToTextFile(docx.c_str(), imported_text.c_str()) != 0 &&
        OpusModernBindPendingDocxUnicode(42) != 0;
    bool cyrillic_sidecar = false;
    bool greek_sidecar = false;
    bool arabic_sidecar = false;
    bool japanese_sidecar = false;
    if (unicode_imported) {
        const std::string imported_bytes = read_file(imported_text);
        for (long cp = 0; cp < static_cast<long>(imported_bytes.size()); ++cp) {
            const unsigned int scalar = OpusUnicodeScalarAt(42, cp);
            if (scalar == 1055) cyrillic_sidecar = true;
            if (scalar == 915) greek_sidecar = true;
            if (scalar == 1605) arabic_sidecar = true;
            if (scalar == 12371) japanese_sidecar = true;
        }
    }
    const bool pdf_written = OpusModernRtfFileToPdf(rtf.c_str(), pdf.c_str()) != 0;
    {
        std::ofstream output(preserved, std::ios::binary);
        output << "ORIGINAL";
    }
    HANDLE oversized_file = CreateFileA(
        oversized.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY, nullptr);
    bool oversized_created = oversized_file != INVALID_HANDLE_VALUE;
    if (oversized_created) {
        LARGE_INTEGER hostile_size{};
        hostile_size.QuadPart = 64ll * 1024ll * 1024ll + 1;
        oversized_created = SetFilePointerEx(oversized_file, hostile_size,
                                             nullptr, FILE_BEGIN) &&
                             SetEndOfFile(oversized_file);
        CloseHandle(oversized_file);
    }
    const bool oversized_rejected = oversized_created &&
        OpusModernRtfFileToPdf(oversized.c_str(), preserved.c_str()) == 0 &&
        read_file(preserved) == "ORIGINAL";
    const bool invalid_snapshot_rejected =
        OpusPdfSnapshotBegin(INT_MAX, INT_MAX, INT_MAX, INT_MAX,
                             INT_MAX, INT_MAX) == 0 &&
        OpusPdfSnapshotBegin(12240, 15840, 1440, 1440, 1440, 1440) != 0 &&
        OpusPdfSnapshotAddParagraph(0, 0, 0, 0, 0, 0, 0,
                                    0, 0, 0, 0) != 0 &&
        OpusPdfSnapshotAddParagraph(0, 0, 0, 0, 0, 0, -240,
                                    0, 0, 0, 0) != 0 &&
        OpusPdfSnapshotAddRun("x", INT_MAX, "Arial", 20,
                              0, 0, 0, 0, 0, 0, 0, 0) == 0;
    std::string result;
    if (read) {
        std::ifstream input(roundtrip, std::ios::binary);
        result.assign(std::istreambuf_iterator<char>(input), {});
    }
    std::string pdf_data;
    if (pdf_written) {
        std::ifstream input(pdf, std::ios::binary);
        pdf_data.assign(std::istreambuf_iterator<char>(input), {});
    }
    DeleteFileA(rtf.c_str());
    DeleteFileA(docx.c_str());
    DeleteFileA(roundtrip.c_str());
    DeleteFileA(imported_text.c_str());
    DeleteFileA(oversized.c_str());
    DeleteFileA(preserved.c_str());
    if (!keep_pdf) DeleteFileA(pdf.c_str());
    if (!written || !read || !unicode_imported || !cyrillic_sidecar ||
        !greek_sidecar || !arabic_sidecar || !japanese_sidecar ||
        result.find("Bold") == std::string::npos ||
        result.find("Second paragraph") == std::string::npos ||
        result.find("\\u1055") == std::string::npos ||
        result.find("\\u915") == std::string::npos ||
        result.find("\\u1605") == std::string::npos ||
        result.find("\\u12371") == std::string::npos ||
        result.find("\\b") == std::string::npos ||
        result.find("\\cf1") == std::string::npos || !pdf_written ||
        !pdf_data.starts_with("%PDF-") ||
        pdf_data.find("Plain") == std::string::npos ||
        pdf_data.find("Bold") == std::string::npos ||
        pdf_data.find("Second paragraph") == std::string::npos ||
        pdf_data.find("/Encoding /Identity-H") == std::string::npos ||
        pdf_data.find("/ToUnicode") == std::string::npos ||
        pdf_data.find("/Helvetica-Bold") == std::string::npos ||
        pdf_data.find("xref") == std::string::npos ||
        pdf_data.find("%%EOF") == std::string::npos ||
        !oversized_rejected || !invalid_snapshot_rejected) {
        std::cerr << "DOCX round trip failed: write=" << written
                  << " read=" << read << " pdf=" << pdf_written
                  << " bytes=" << result.size()
                  << " oversizedRejected=" << oversized_rejected
                  << " invalidSnapshotRejected="
                  << invalid_snapshot_rejected << '\n';
        return 2;
    }
    std::cout << "DOCX round trip passed (" << result.size() << " RTF bytes)\n";
    return 0;
}
