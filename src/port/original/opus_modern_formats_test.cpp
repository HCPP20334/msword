#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <fstream>
#include <iostream>
#include <string>

extern "C" int OpusModernDocxToRtfFile(const char*, const char*);
extern "C" int OpusModernRtfFileToDocx(const char*, const char*);
extern "C" int OpusModernRtfFileToPdf(const char*, const char*);

int main(const int argument_count, char** arguments) {
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
                  "{\\ul Underline} {\\cf1 Red}\\par Second paragraph}";
    }
    const bool written = OpusModernRtfFileToDocx(rtf.c_str(), docx.c_str()) != 0;
    const bool read = written &&
        OpusModernDocxToRtfFile(docx.c_str(), roundtrip.c_str()) != 0;
    const bool pdf_written = OpusModernRtfFileToPdf(rtf.c_str(), pdf.c_str()) != 0;
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
    if (!keep_pdf) DeleteFileA(pdf.c_str());
    if (!written || !read || result.find("Bold") == std::string::npos ||
        result.find("Second paragraph") == std::string::npos ||
        result.find("\\b") == std::string::npos ||
        result.find("\\cf1") == std::string::npos || !pdf_written ||
        !pdf_data.starts_with("%PDF-") ||
        pdf_data.find("Plain") == std::string::npos ||
        pdf_data.find("Bold") == std::string::npos ||
        pdf_data.find("Second paragraph") == std::string::npos ||
        pdf_data.find("/Helvetica-Bold") == std::string::npos ||
        pdf_data.find("xref") == std::string::npos ||
        pdf_data.find("%%EOF") == std::string::npos) {
        std::cerr << "DOCX round trip failed: write=" << written
                  << " read=" << read << " pdf=" << pdf_written
                  << " bytes=" << result.size() << '\n';
        return 2;
    }
    std::cout << "DOCX round trip passed (" << result.size() << " RTF bytes)\n";
    return 0;
}
