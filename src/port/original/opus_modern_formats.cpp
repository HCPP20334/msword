#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <msopc.h>
#include <richedit.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace {

struct ComApartment {
    HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    ~ComApartment() {
        if (result == S_OK || result == S_FALSE) {
            CoUninitialize();
        }
    }
    bool usable() const {
        return SUCCEEDED(result) || result == RPC_E_CHANGED_MODE;
    }
};

std::wstring wide_path(const char* path) {
    if (path == nullptr || *path == '\0') {
        return {};
    }
    const int count = MultiByteToWideChar(CP_ACP, 0, path, -1, nullptr, 0);
    std::wstring result(count > 0 ? static_cast<std::size_t>(count) : 0,
                        L'\0');
    if (count > 1) {
        MultiByteToWideChar(CP_ACP, 0, path, -1, result.data(), count);
        result.resize(static_cast<std::size_t>(count - 1));
    }
    return result;
}

std::wstring utf8_to_wide(std::string_view value) {
    if (value.empty()) {
        return {};
    }
    const int count = MultiByteToWideChar(CP_UTF8, 0, value.data(),
                                           static_cast<int>(value.size()),
                                           nullptr, 0);
    std::wstring result(count > 0 ? static_cast<std::size_t>(count) : 0,
                        L'\0');
    if (count > 0) {
        MultiByteToWideChar(CP_UTF8, 0, value.data(),
                            static_cast<int>(value.size()), result.data(),
                            count);
    }
    return result;
}

std::string wide_to_utf8(std::wstring_view value) {
    if (value.empty()) {
        return {};
    }
    const int count = WideCharToMultiByte(CP_UTF8, 0, value.data(),
                                          static_cast<int>(value.size()),
                                          nullptr, 0, nullptr, nullptr);
    std::string result(count > 0 ? static_cast<std::size_t>(count) : 0, '\0');
    if (count > 0) {
        WideCharToMultiByte(CP_UTF8, 0, value.data(),
                            static_cast<int>(value.size()), result.data(),
                            count, nullptr, nullptr);
    }
    return result;
}

std::string wide_to_ansi(std::wstring_view value) {
    if (value.empty()) return {};
    const int count = WideCharToMultiByte(
        1252, WC_NO_BEST_FIT_CHARS, value.data(),
        static_cast<int>(value.size()), nullptr, 0, "?", nullptr);
    std::string result(count > 0 ? static_cast<std::size_t>(count) : 0, '\0');
    if (count > 0) {
        WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, value.data(),
                            static_cast<int>(value.size()), result.data(),
                            count, "?", nullptr);
    }
    return result;
}

std::string xml_escape(std::wstring_view text) {
    std::string result;
    for (const wchar_t character : text) {
        switch (character) {
            case L'&': result += "&amp;"; break;
            case L'<': result += "&lt;"; break;
            case L'>': result += "&gt;"; break;
            case L'\"': result += "&quot;"; break;
            case L'\'': result += "&apos;"; break;
            default: result += wide_to_utf8(std::wstring_view(&character, 1));
        }
    }
    return result;
}

std::wstring xml_unescape(std::string_view text) {
    std::string decoded;
    for (std::size_t position = 0; position < text.size();) {
        if (text[position] != '&') {
            decoded.push_back(text[position++]);
            continue;
        }
        const std::size_t end = text.find(';', position + 1);
        if (end == std::string_view::npos) {
            decoded.push_back(text[position++]);
            continue;
        }
        const std::string entity(text.substr(position + 1, end - position - 1));
        if (entity == "amp") decoded.push_back('&');
        else if (entity == "lt") decoded.push_back('<');
        else if (entity == "gt") decoded.push_back('>');
        else if (entity == "quot") decoded.push_back('"');
        else if (entity == "apos") decoded.push_back('\'');
        else if (!entity.empty() && entity[0] == '#') {
            unsigned long code = 0;
            char* parse_end = nullptr;
            const char* digits = entity.c_str() + 1;
            int radix = 10;
            if (entity.size() > 2 && (entity[1] == 'x' || entity[1] == 'X')) {
                digits = entity.c_str() + 2;
                radix = 16;
            }
            code = std::strtoul(digits, &parse_end, radix);
            if (parse_end != digits && *parse_end == '\0' &&
                code <= 0x10ffff && !(code >= 0xd800 && code <= 0xdfff)) {
                if (code <= 0xffff) {
                    const wchar_t scalar = static_cast<wchar_t>(code);
                    decoded += wide_to_utf8(std::wstring_view(&scalar, 1));
                } else {
                    const unsigned long scalar = code - 0x10000;
                    const wchar_t pair[2] = {
                        static_cast<wchar_t>(0xd800 + (scalar >> 10)),
                        static_cast<wchar_t>(0xdc00 + (scalar & 0x3ff))};
                    decoded += wide_to_utf8(std::wstring_view(pair, 2));
                }
            }
        } else {
            decoded.append(text.substr(position, end - position + 1));
        }
        position = end + 1;
    }
    return utf8_to_wide(decoded);
}

bool has_extension(const std::string& path, const char* extension) {
    const std::size_t length = std::strlen(extension);
    return path.size() >= length &&
           _stricmp(path.c_str() + path.size() - length, extension) == 0;
}

bool read_stream(IStream* stream, std::string& data) {
    STATSTG status{};
    if (stream == nullptr || FAILED(stream->Stat(&status, STATFLAG_NONAME)) ||
        status.cbSize.QuadPart < 0 ||
        status.cbSize.QuadPart > static_cast<LONGLONG>(256 * 1024 * 1024)) {
        return false;
    }
    data.resize(static_cast<std::size_t>(status.cbSize.QuadPart));
    LARGE_INTEGER zero{};
    stream->Seek(zero, STREAM_SEEK_SET, nullptr);
    ULONG read = 0;
    return data.empty() ||
           (SUCCEEDED(stream->Read(data.data(), static_cast<ULONG>(data.size()),
                                   &read)) && read == data.size());
}

bool read_opc_part(const std::wstring& path, const wchar_t* part_name,
                   std::string& data) {
    ComApartment apartment;
    if (!apartment.usable()) return false;
    ComPtr<IOpcFactory> factory;
    ComPtr<IStream> file;
    ComPtr<IOpcPackage> package;
    ComPtr<IOpcPartSet> parts;
    ComPtr<IOpcPartUri> uri;
    ComPtr<IOpcPart> part;
    ComPtr<IStream> content;
    return SUCCEEDED(CoCreateInstance(__uuidof(OpcFactory), nullptr,
                                      CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(&factory))) &&
           SUCCEEDED(factory->CreateStreamOnFile(path.c_str(),
                                                 OPC_STREAM_IO_READ, nullptr,
                                                 FILE_ATTRIBUTE_NORMAL,
                                                 &file)) &&
           SUCCEEDED(factory->ReadPackageFromStream(file.Get(),
                                                    OPC_READ_DEFAULT,
                                                    &package)) &&
           SUCCEEDED(package->GetPartSet(&parts)) &&
           SUCCEEDED(factory->CreatePartUri(part_name, &uri)) &&
           SUCCEEDED(parts->GetPart(uri.Get(), &part)) &&
           SUCCEEDED(part->GetContentStream(&content)) &&
           read_stream(content.Get(), data);
}

std::string tag_attribute(std::string_view tag, std::string_view name) {
    std::size_t position = 0;
    while ((position = tag.find(name, position)) != std::string_view::npos) {
        const bool boundary = position == 0 ||
            std::isspace(static_cast<unsigned char>(tag[position - 1])) ||
            tag[position - 1] == ':';
        std::size_t equals = position + name.size();
        while (equals < tag.size() && std::isspace(
                   static_cast<unsigned char>(tag[equals]))) ++equals;
        if (!boundary || equals >= tag.size() || tag[equals] != '=') {
            position += name.size();
            continue;
        }
        ++equals;
        while (equals < tag.size() && std::isspace(
                   static_cast<unsigned char>(tag[equals]))) ++equals;
        if (equals >= tag.size() || (tag[equals] != '\'' && tag[equals] != '"'))
            return {};
        const char quote = tag[equals++];
        const std::size_t end = tag.find(quote, equals);
        return end == std::string_view::npos ? std::string{} :
            std::string(tag.substr(equals, end - equals));
    }
    return {};
}

std::string local_tag_name(std::string_view tag) {
    std::size_t start = 0;
    while (start < tag.size() && (tag[start] == '<' || tag[start] == '/' ||
           std::isspace(static_cast<unsigned char>(tag[start])))) ++start;
    std::size_t end = start;
    while (end < tag.size() && !std::isspace(static_cast<unsigned char>(tag[end])) &&
           tag[end] != '/' && tag[end] != '>') ++end;
    const std::size_t colon = tag.substr(start, end - start).rfind(':');
    if (colon != std::string_view::npos) start += colon + 1;
    return std::string(tag.substr(start, end - start));
}

struct RunStyle {
    bool bold = false;
    bool italic = false;
    bool underline = false;
    int half_points = 20;
    COLORREF color = RGB(0, 0, 0);
    bool auto_color = true;
    std::wstring font = L"Arial";
    bool operator==(const RunStyle&) const = default;
};

struct TextRun { RunStyle style; std::wstring text; };
struct Paragraph { int alignment = PFA_LEFT; std::vector<TextRun> runs; };

bool property_enabled(std::string_view properties, const char* local_name) {
    std::size_t position = 0;
    while ((position = properties.find('<', position)) != std::string_view::npos) {
        const std::size_t end = properties.find('>', position + 1);
        if (end == std::string_view::npos) break;
        const std::string_view tag = properties.substr(position, end - position + 1);
        if (local_tag_name(tag) == local_name) {
            const std::string value = tag_attribute(tag, "val");
            return value.empty() || (value != "0" && value != "false" &&
                                     value != "none");
        }
        position = end + 1;
    }
    return false;
}

std::string first_property_value(std::string_view properties,
                                 const char* local_name,
                                 const char* attribute = "val") {
    std::size_t position = 0;
    while ((position = properties.find('<', position)) != std::string_view::npos) {
        const std::size_t end = properties.find('>', position + 1);
        if (end == std::string_view::npos) break;
        const std::string_view tag = properties.substr(position, end - position + 1);
        if (local_tag_name(tag) == local_name) return tag_attribute(tag, attribute);
        position = end + 1;
    }
    return {};
}

RunStyle parse_run_style(std::string_view run) {
    RunStyle style;
    const std::size_t start = run.find("<w:rPr");
    const std::size_t end = start == std::string_view::npos ?
        std::string_view::npos : run.find("</w:rPr>", start);
    if (start == std::string_view::npos || end == std::string_view::npos) return style;
    const std::string_view props = run.substr(start, end + 8 - start);
    style.bold = property_enabled(props, "b");
    style.italic = property_enabled(props, "i");
    style.underline = property_enabled(props, "u");
    if (const std::string size = first_property_value(props, "sz"); !size.empty()) {
        style.half_points = (std::max)(2, std::atoi(size.c_str()));
    }
    if (const std::string font = first_property_value(props, "rFonts", "ascii");
        !font.empty()) {
        style.font = xml_unescape(font);
    }
    if (const std::string color = first_property_value(props, "color");
        !color.empty() && color != "auto" && color.size() == 6) {
        const unsigned rgb = std::strtoul(color.c_str(), nullptr, 16);
        style.color = RGB((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff);
        style.auto_color = false;
    }
    return style;
}

std::wstring parse_run_text(std::string_view run) {
    std::wstring text;
    std::size_t position = 0;
    while ((position = run.find('<', position)) != std::string_view::npos) {
        const std::size_t tag_end = run.find('>', position + 1);
        if (tag_end == std::string_view::npos) break;
        const std::string_view tag = run.substr(position, tag_end - position + 1);
        const std::string name = local_tag_name(tag);
        if (name == "t") {
            const std::size_t close = run.find("</w:t>", tag_end + 1);
            if (close == std::string_view::npos) break;
            text += xml_unescape(run.substr(tag_end + 1, close - tag_end - 1));
            position = close + 6;
        } else {
            if (name == "tab") text.push_back(L'\t');
            else if (name == "br" || name == "cr") text.push_back(L'\n');
            position = tag_end + 1;
        }
    }
    return text;
}

std::vector<Paragraph> parse_document_xml(std::string_view xml) {
    std::vector<Paragraph> paragraphs;
    std::size_t position = 0;
    while ((position = xml.find("<w:p", position)) != std::string_view::npos) {
        const char next = position + 4 < xml.size() ? xml[position + 4] : '\0';
        if (next != '>' && next != '/' && !std::isspace(static_cast<unsigned char>(next))) {
            position += 4;
            continue;
        }
        const std::size_t open_end = xml.find('>', position);
        const std::size_t close = xml.find("</w:p>", open_end);
        if (open_end == std::string_view::npos || close == std::string_view::npos) break;
        const std::string_view block = xml.substr(open_end + 1, close - open_end - 1);
        Paragraph paragraph;
        const std::string alignment = first_property_value(block, "jc");
        if (alignment == "center") paragraph.alignment = PFA_CENTER;
        else if (alignment == "right") paragraph.alignment = PFA_RIGHT;
        else if (alignment == "both" || alignment == "distribute")
            paragraph.alignment = PFA_JUSTIFY;

        std::size_t run_position = 0;
        while ((run_position = block.find("<w:r", run_position)) != std::string_view::npos) {
            const char run_next = run_position + 4 < block.size() ? block[run_position + 4] : '\0';
            if (run_next != '>' && run_next != '/' &&
                !std::isspace(static_cast<unsigned char>(run_next))) {
                run_position += 4;
                continue;
            }
            const std::size_t run_open_end = block.find('>', run_position);
            const std::size_t run_close = block.find("</w:r>", run_open_end);
            if (run_open_end == std::string_view::npos || run_close == std::string_view::npos)
                break;
            const std::string_view run = block.substr(
                run_position, run_close + 6 - run_position);
            std::wstring text = parse_run_text(run);
            if (!text.empty()) paragraph.runs.push_back({parse_run_style(run), std::move(text)});
            run_position = run_close + 6;
        }
        paragraphs.push_back(std::move(paragraph));
        position = close + 6;
    }
    if (paragraphs.empty()) paragraphs.push_back({});
    return paragraphs;
}

std::string paragraphs_to_text(const std::vector<Paragraph>& paragraphs) {
    std::string text;
    for (std::size_t index = 0; index < paragraphs.size(); ++index) {
        for (const auto& run : paragraphs[index].runs) {
            text += wide_to_ansi(run.text);
        }
        if (index + 1 < paragraphs.size()) text += "\r\n";
    }
    return text;
}

std::string ansi_text_to_rtf(std::string_view text) {
    std::string rtf =
        "{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0 Arial;}}\\f0\\fs24 ";
    static constexpr char hex[] = "0123456789abcdef";
    for (std::size_t index = 0; index < text.size(); ++index) {
        const unsigned char character =
            static_cast<unsigned char>(text[index]);
        if (character == '\r' || character == '\n') {
            if (character == '\r' && index + 1 < text.size() &&
                text[index + 1] == '\n') {
                ++index;
            }
            rtf += "\\par ";
        } else if (character == '\t') {
            rtf += "\\tab ";
        } else if (character == '\f') {
            rtf += "\\page ";
        } else if (character == '\\' || character == '{' ||
                   character == '}') {
            rtf.push_back('\\');
            rtf.push_back(static_cast<char>(character));
        } else if (character >= 0x80) {
            rtf += "\\'";
            rtf.push_back(hex[character >> 4]);
            rtf.push_back(hex[character & 0x0f]);
        } else if (character >= 0x20) {
            rtf.push_back(static_cast<char>(character));
        }
    }
    rtf.push_back('}');
    return rtf;
}

void append_rtf_text(std::string& rtf, std::wstring_view text) {
    for (const wchar_t character : text) {
        if (character == L'\\' || character == L'{' || character == L'}') {
            rtf.push_back('\\');
            rtf.push_back(static_cast<char>(character));
        } else if (character == L'\t') rtf += "\\tab ";
        else if (character == L'\n') rtf += "\\line ";
        else if (character >= 0x20 && character <= 0x7e) {
            rtf.push_back(static_cast<char>(character));
        } else {
            char encoded = '?';
            WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, &character, 1,
                                &encoded, 1, "?", nullptr);
            static constexpr char hex[] = "0123456789abcdef";
            const unsigned byte = static_cast<unsigned char>(encoded);
            rtf += "\\'";
            rtf.push_back(hex[byte >> 4]);
            rtf.push_back(hex[byte & 0x0f]);
        }
    }
}

std::string paragraphs_to_rtf(const std::vector<Paragraph>& paragraphs) {
    std::vector<std::wstring> fonts{L"Arial"};
    std::vector<COLORREF> colors;
    for (const auto& paragraph : paragraphs) for (const auto& run : paragraph.runs) {
        if (std::find(fonts.begin(), fonts.end(), run.style.font) == fonts.end())
            fonts.push_back(run.style.font);
        if (!run.style.auto_color &&
            std::find(colors.begin(), colors.end(), run.style.color) == colors.end())
            colors.push_back(run.style.color);
    }
    std::string rtf = "{\\rtf1\\ansi \\deff0";
    rtf += "{\\fonttbl";
    for (std::size_t index = 0; index < fonts.size(); ++index) {
        rtf += "{\\f" + std::to_string(index) + "\\fnil ";
        append_rtf_text(rtf, fonts[index]);
        rtf += ";}";
    }
    rtf += "}{\\colortbl;";
    for (const COLORREF color : colors) {
        rtf += "\\red" + std::to_string(GetRValue(color)) +
               "\\green" + std::to_string(GetGValue(color)) +
               "\\blue" + std::to_string(GetBValue(color)) + ";";
    }
    rtf += "}";
    for (const auto& paragraph : paragraphs) {
        rtf += "\\pard";
        if (paragraph.alignment == PFA_CENTER) rtf += "\\qc";
        else if (paragraph.alignment == PFA_RIGHT) rtf += "\\qr";
        else if (paragraph.alignment == PFA_JUSTIFY) rtf += "\\qj";
        else rtf += "\\ql";
        rtf.push_back(' ');
        for (const auto& run : paragraph.runs) {
            const auto font = std::find(fonts.begin(), fonts.end(), run.style.font);
            const auto color = std::find(colors.begin(), colors.end(), run.style.color);
            rtf += "{";
            rtf += run.style.bold ? "\\b" : "\\b0";
            rtf += run.style.italic ? "\\i" : "\\i0";
            rtf += run.style.underline ? "\\ul" : "\\ulnone";
            rtf += "\\fs" + std::to_string(run.style.half_points);
            rtf += "\\f" + std::to_string(std::distance(fonts.begin(), font));
            if (!run.style.auto_color)
                rtf += "\\cf" + std::to_string(std::distance(colors.begin(), color) + 1);
            rtf.push_back(' ');
            append_rtf_text(rtf, run.text);
            rtf += "}";
        }
        rtf += "\\par\n";
    }
    rtf += "}";
    return rtf;
}

bool write_bytes(const std::wstring& path, std::string_view bytes) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    const bool ok = bytes.size() <= MAXDWORD &&
        WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()),
                  &written, nullptr) && written == bytes.size();
    CloseHandle(file);
    if (!ok) DeleteFileW(path.c_str());
    return ok;
}

bool read_bytes(const std::wstring& path, std::string& bytes) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    bool ok = GetFileSizeEx(file, &size) && size.QuadPart >= 0 &&
              size.QuadPart <= 256 * 1024 * 1024;
    if (ok) {
        bytes.resize(static_cast<std::size_t>(size.QuadPart));
        DWORD read = 0;
        ok = bytes.empty() || (ReadFile(file, bytes.data(),
            static_cast<DWORD>(bytes.size()), &read, nullptr) && read == bytes.size());
    }
    CloseHandle(file);
    return ok;
}

struct StreamCookie { const char* data; LONG length; LONG position; };
DWORD CALLBACK rich_edit_stream_in(DWORD_PTR cookie, LPBYTE buffer,
                                   LONG requested, LONG* copied) {
    auto& source = *reinterpret_cast<StreamCookie*>(cookie);
    *copied = (std::min)(requested, source.length - source.position);
    if (*copied > 0) {
        std::memcpy(buffer, source.data + source.position, *copied);
        source.position += *copied;
    }
    return 0;
}

class RichEditDocument {
public:
    bool load(std::string_view rtf) {
        module_ = LoadLibraryW(L"Msftedit.dll");
        if (module_ == nullptr) return false;
        window_ = CreateWindowExW(0, MSFTEDIT_CLASS, L"", WS_POPUP | ES_MULTILINE,
                                  0, 0, 100, 100, nullptr, nullptr,
                                  GetModuleHandleW(nullptr), nullptr);
        if (window_ == nullptr) return false;
        SendMessageW(window_, EM_EXLIMITTEXT, 0, 0x7ffffffe);
        StreamCookie cookie{rtf.data(), static_cast<LONG>(rtf.size()), 0};
        EDITSTREAM stream{reinterpret_cast<DWORD_PTR>(&cookie), 0,
                          rich_edit_stream_in};
        SendMessageW(window_, EM_STREAMIN, SF_RTF,
                     reinterpret_cast<LPARAM>(&stream));
        return stream.dwError == 0;
    }
    ~RichEditDocument() {
        if (window_ != nullptr) DestroyWindow(window_);
        if (module_ != nullptr) FreeLibrary(module_);
    }
    HWND window() const { return window_; }
    std::wstring text() const {
        const int length = GetWindowTextLengthW(window_);
        std::wstring value(length > 0 ? static_cast<std::size_t>(length + 1) : 0,
                           L'\0');
        if (length > 0) {
            GetWindowTextW(window_, value.data(), length + 1);
            value.resize(static_cast<std::size_t>(length));
        }
        return value;
    }
private:
    HMODULE module_ = nullptr;
    HWND window_ = nullptr;
};

RunStyle rich_style_at(HWND rich, LONG position) {
    CHARRANGE range{position, position + 1};
    SendMessageW(rich, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&range));
    CHARFORMAT2W format{};
    format.cbSize = sizeof(format);
    SendMessageW(rich, EM_GETCHARFORMAT, SCF_SELECTION,
                 reinterpret_cast<LPARAM>(&format));
    RunStyle style;
    style.bold = (format.dwEffects & CFE_BOLD) != 0;
    style.italic = (format.dwEffects & CFE_ITALIC) != 0;
    style.underline = (format.dwEffects & CFE_UNDERLINE) != 0;
    style.half_points = format.yHeight > 0 ?
        (std::max)(2L, format.yHeight / 10) : 20;
    style.auto_color = (format.dwEffects & CFE_AUTOCOLOR) != 0;
    style.color = format.crTextColor;
    if (format.szFaceName[0] != L'\0') style.font = format.szFaceName;
    return style;
}

int rich_alignment_at(HWND rich, LONG position) {
    CHARRANGE range{position, position};
    SendMessageW(rich, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&range));
    PARAFORMAT2 format{};
    format.cbSize = sizeof(format);
    format.dwMask = PFM_ALIGNMENT;
    SendMessageW(rich, EM_GETPARAFORMAT, 0, reinterpret_cast<LPARAM>(&format));
    return format.wAlignment;
}

std::vector<Paragraph> paragraphs_from_rich_edit(RichEditDocument& rich) {
    const std::wstring text = rich.text();
    std::vector<Paragraph> paragraphs;
    LONG paragraph_start = 0;
    while (paragraph_start <= static_cast<LONG>(text.size())) {
        LONG paragraph_end = paragraph_start;
        while (paragraph_end < static_cast<LONG>(text.size()) &&
               text[paragraph_end] != L'\r' && text[paragraph_end] != L'\n')
            ++paragraph_end;
        Paragraph paragraph;
        paragraph.alignment = rich_alignment_at(rich.window(), paragraph_start);
        LONG run_start = paragraph_start;
        while (run_start < paragraph_end) {
            RunStyle style = rich_style_at(rich.window(), run_start);
            LONG run_end = run_start + 1;
            while (run_end < paragraph_end &&
                   rich_style_at(rich.window(), run_end) == style) ++run_end;
            paragraph.runs.push_back({style, text.substr(
                static_cast<std::size_t>(run_start),
                static_cast<std::size_t>(run_end - run_start))});
            run_start = run_end;
        }
        paragraphs.push_back(std::move(paragraph));
        if (paragraph_end >= static_cast<LONG>(text.size())) break;
        paragraph_start = paragraph_end + 1;
        if (paragraph_start < static_cast<LONG>(text.size()) &&
            text[paragraph_end] == L'\r' && text[paragraph_start] == L'\n')
            ++paragraph_start;
    }
    return paragraphs;
}

std::string document_xml(const std::vector<Paragraph>& paragraphs) {
    std::string xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
        "<w:body>";
    for (const auto& paragraph : paragraphs) {
        xml += "<w:p>";
        if (paragraph.alignment != PFA_LEFT) {
            const char* value = paragraph.alignment == PFA_CENTER ? "center" :
                paragraph.alignment == PFA_RIGHT ? "right" : "both";
            xml += std::string("<w:pPr><w:jc w:val=\"") + value + "\"/></w:pPr>";
        }
        for (const auto& run : paragraph.runs) {
            xml += "<w:r><w:rPr>";
            if (run.style.bold) xml += "<w:b/>";
            if (run.style.italic) xml += "<w:i/>";
            if (run.style.underline) xml += "<w:u w:val=\"single\"/>";
            xml += "<w:rFonts w:ascii=\"" + xml_escape(run.style.font) +
                   "\" w:hAnsi=\"" + xml_escape(run.style.font) + "\"/>";
            xml += "<w:sz w:val=\"" + std::to_string(run.style.half_points) + "\"/>";
            if (!run.style.auto_color) {
                char color[7]{};
                wsprintfA(color, "%02X%02X%02X", GetRValue(run.style.color),
                          GetGValue(run.style.color), GetBValue(run.style.color));
                xml += std::string("<w:color w:val=\"") + color + "\"/>";
            }
            xml += "</w:rPr><w:t xml:space=\"preserve\">" +
                   xml_escape(run.text) + "</w:t></w:r>";
        }
        xml += "</w:p>";
    }
    xml += "<w:sectPr><w:pgSz w:w=\"12240\" w:h=\"15840\"/>"
           "<w:pgMar w:top=\"1440\" w:right=\"1440\" w:bottom=\"1440\" w:left=\"1440\"/>"
           "</w:sectPr></w:body></w:document>";
    return xml;
}

bool add_part(IOpcFactory* factory, IOpcPartSet* parts, const wchar_t* name,
              const wchar_t* content_type, std::string_view data,
              IOpcPart** created = nullptr) {
    ComPtr<IOpcPartUri> uri;
    ComPtr<IOpcPart> part;
    ComPtr<IStream> stream;
    ULONG written = 0;
    if (FAILED(factory->CreatePartUri(name, &uri)) ||
        FAILED(parts->CreatePart(uri.Get(), content_type,
                                 OPC_COMPRESSION_NORMAL, &part)) ||
        FAILED(part->GetContentStream(&stream)) ||
        (!data.empty() && (FAILED(stream->Write(data.data(),
            static_cast<ULONG>(data.size()), &written)) || written != data.size())))
        return false;
    if (created != nullptr) *created = part.Detach();
    return true;
}

bool add_relationship(IOpcRelationshipSet* set, IOpcFactory* factory,
                      const wchar_t* target, const wchar_t* type) {
    ComPtr<IOpcPartUri> uri;
    ComPtr<IOpcRelationship> relationship;
    return SUCCEEDED(factory->CreatePartUri(target, &uri)) &&
           SUCCEEDED(set->CreateRelationship(nullptr, type, uri.Get(),
                                             OPC_URI_TARGET_MODE_INTERNAL,
                                             &relationship));
}

bool write_docx(const std::wstring& path, const std::vector<Paragraph>& paragraphs) {
    ComApartment apartment;
    if (!apartment.usable()) return false;
    ComPtr<IOpcFactory> factory;
    ComPtr<IOpcPackage> package;
    ComPtr<IOpcPartSet> parts;
    ComPtr<IOpcPart> main_part;
    ComPtr<IOpcRelationshipSet> package_relationships;
    ComPtr<IOpcRelationshipSet> document_relationships;
    ComPtr<IStream> output;
    if (FAILED(CoCreateInstance(__uuidof(OpcFactory), nullptr,
                                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))) ||
        FAILED(factory->CreatePackage(&package)) ||
        FAILED(package->GetPartSet(&parts)) ||
        !add_part(factory.Get(), parts.Get(), L"/word/document.xml",
                  L"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml",
                  document_xml(paragraphs), &main_part)) return false;

    const std::string styles =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<w:styles xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
        "<w:style w:type=\"paragraph\" w:default=\"1\" w:styleId=\"Normal\">"
        "<w:name w:val=\"Normal\"/><w:qFormat/></w:style></w:styles>";
    const std::string settings =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<w:settings xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\"/>";
    const std::string core =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<cp:coreProperties xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" "
        "xmlns:dc=\"http://purl.org/dc/elements/1.1/\"><dc:creator>Microsoft Word</dc:creator>"
        "<dc:title>Document</dc:title></cp:coreProperties>";
    const std::string app =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Properties xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\">"
        "<Application>Microsoft Word</Application></Properties>";
    if (!add_part(factory.Get(), parts.Get(), L"/word/styles.xml",
                  L"application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml", styles) ||
        !add_part(factory.Get(), parts.Get(), L"/word/settings.xml",
                  L"application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml", settings) ||
        !add_part(factory.Get(), parts.Get(), L"/docProps/core.xml",
                  L"application/vnd.openxmlformats-package.core-properties+xml", core) ||
        !add_part(factory.Get(), parts.Get(), L"/docProps/app.xml",
                  L"application/vnd.openxmlformats-officedocument.extended-properties+xml", app) ||
        FAILED(package->GetRelationshipSet(&package_relationships)) ||
        !add_relationship(package_relationships.Get(), factory.Get(),
                          L"/word/document.xml",
                          L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument") ||
        !add_relationship(package_relationships.Get(), factory.Get(),
                          L"/docProps/core.xml",
                          L"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties") ||
        !add_relationship(package_relationships.Get(), factory.Get(),
                          L"/docProps/app.xml",
                          L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties") ||
        FAILED(main_part->GetRelationshipSet(&document_relationships)) ||
        !add_relationship(document_relationships.Get(), factory.Get(),
                          L"/word/styles.xml",
                          L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles") ||
        !add_relationship(document_relationships.Get(), factory.Get(),
                          L"/word/settings.xml",
                          L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings"))
        return false;
    DeleteFileW(path.c_str());
    return SUCCEEDED(factory->CreateStreamOnFile(path.c_str(), OPC_STREAM_IO_WRITE,
                                                  nullptr, FILE_ATTRIBUTE_NORMAL,
                                                  &output)) &&
           SUCCEEDED(factory->WritePackageToStream(package.Get(),
                                                   OPC_WRITE_DEFAULT,
                                                   output.Get()));
}

bool rtf_to_docx(const std::wstring& rtf_path, const std::wstring& docx_path) {
    std::string rtf;
    RichEditDocument rich;
    return read_bytes(rtf_path, rtf) && rich.load(rtf) &&
           write_docx(docx_path, paragraphs_from_rich_edit(rich));
}

struct PdfLineRun {
    RunStyle style;
    std::wstring text;
    double width = 0.0;
};

struct PdfLine {
    std::vector<PdfLineRun> runs;
    double width = 0.0;
    double largest_font = 12.0;
    int alignment = PFA_LEFT;
};

std::string pdf_number(const double value) {
    char buffer[64]{};
    std::snprintf(buffer, std::size(buffer), "%.3f", value);
    std::string result = buffer;
    while (result.size() > 1 && result.back() == '0') result.pop_back();
    if (!result.empty() && result.back() == '.') result.pop_back();
    return result;
}

double pdf_font_size(const RunStyle& style) {
    return (std::max)(4.0, static_cast<double>(style.half_points) / 2.0);
}

double pdf_character_width(const wchar_t character, const RunStyle& style) {
    double em = 0.556;
    if (character == L' ') em = 0.278;
    else if (character == L'i' || character == L'l' || character == L'I' ||
             character == L'.' || character == L',' || character == L'!' ||
             character == L'\'' || character == L'`' || character == L'|' ||
             character == L':' || character == L';') em = 0.278;
    else if (character == L'm' || character == L'w' || character == L'M' ||
             character == L'W' || character == L'@' || character == L'%' ||
             character == L'&') em = 0.889;
    else if (character >= L'A' && character <= L'Z') em = 0.667;
    else if (character >= L'a' && character <= L'z') em = 0.500;
    else if (character == L'(' || character == L')' || character == L'[' ||
             character == L']' || character == L'{' || character == L'}')
        em = 0.333;
    if (style.bold) em *= 1.03;
    return em * pdf_font_size(style);
}

std::wstring lower_ascii(std::wstring value) {
    for (wchar_t& character : value) {
        if (character >= L'A' && character <= L'Z') character += L'a' - L'A';
    }
    return value;
}

int pdf_font_resource(const RunStyle& style) {
    const std::wstring font = lower_ascii(style.font);
    int family = 0;
    if (font.find(L"times") != std::wstring::npos ||
        font.find(L"roman") != std::wstring::npos ||
        font.find(L"serif") != std::wstring::npos) {
        family = 1;
    } else if (font.find(L"courier") != std::wstring::npos ||
               font.find(L"mono") != std::wstring::npos) {
        family = 2;
    }
    const int face = (style.bold ? 1 : 0) + (style.italic ? 2 : 0);
    return family * 4 + face + 1;
}

std::string pdf_encoded_text(std::wstring_view text) {
    std::string encoded;
    for (const wchar_t character : text) {
        char byte = '?';
        BOOL used_default = FALSE;
        if (character >= 0x20) {
            WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, &character, 1,
                                &byte, 1, "?", &used_default);
        } else {
            continue;
        }
        if (byte == '\\' || byte == '(' || byte == ')') encoded.push_back('\\');
        encoded.push_back(byte);
    }
    return encoded;
}

void append_pdf_line(std::string& content, const PdfLine& line,
                     const double baseline) {
    constexpr double left = 72.0;
    constexpr double writable_width = 468.0;
    double x = left;
    if (line.alignment == PFA_CENTER) {
        x += ((std::max)(0.0, writable_width - line.width)) / 2.0;
    } else if (line.alignment == PFA_RIGHT) {
        x += (std::max)(0.0, writable_width - line.width);
    }
    std::string underlines;
    content += "BT\n1 0 0 1 " + pdf_number(x) + " " +
               pdf_number(baseline) + " Tm\n";
    for (const PdfLineRun& run : line.runs) {
        const double font_size = pdf_font_size(run.style);
        const COLORREF color = run.style.auto_color ? RGB(0, 0, 0) :
                                                     run.style.color;
        const double red = static_cast<double>(GetRValue(color)) / 255.0;
        const double green = static_cast<double>(GetGValue(color)) / 255.0;
        const double blue = static_cast<double>(GetBValue(color)) / 255.0;
        content += "/F" + std::to_string(pdf_font_resource(run.style)) + " " +
                   pdf_number(font_size) + " Tf\n";
        content += pdf_number(red) + " " + pdf_number(green) + " " +
                   pdf_number(blue) + " rg\n(" +
                   pdf_encoded_text(run.text) + ") Tj\n";
        if (run.style.underline && run.width > 0.0) {
            const double underline_y = baseline - (std::max)(1.0, font_size / 12.0);
            underlines += "q\n" + pdf_number(red) + " " + pdf_number(green) +
                          " " + pdf_number(blue) + " RG\n" +
                          pdf_number((std::max)(0.5, font_size / 18.0)) +
                          " w\n" + pdf_number(x) + " " +
                          pdf_number(underline_y) + " m\n" +
                          pdf_number(x + run.width) + " " +
                          pdf_number(underline_y) + " l\nS\nQ\n";
        }
        x += run.width;
    }
    content += "ET\n" + underlines;
}

std::vector<std::string> pdf_page_contents(
    const std::vector<Paragraph>& paragraphs) {
    constexpr double top = 720.0;
    constexpr double bottom = 72.0;
    constexpr double writable_width = 468.0;
    std::vector<std::string> pages(1);
    double cursor = top;
    PdfLine line;

    const auto begin_new_page = [&]() {
        pages.emplace_back();
        cursor = top;
    };
    const auto emit_line = [&](const bool force) {
        if (!force && line.runs.empty()) return;
        const double line_height = (std::max)(12.0, line.largest_font * 1.20);
        if (cursor - line_height < bottom) begin_new_page();
        const double baseline = cursor - line.largest_font;
        append_pdf_line(pages.back(), line, baseline);
        cursor -= line_height;
        const int alignment = line.alignment;
        line = PdfLine{};
        line.alignment = alignment;
    };
    const auto add_character = [&](const wchar_t character,
                                   const RunStyle& style) {
        const double width = pdf_character_width(character, style);
        if (!line.runs.empty() && line.width + width > writable_width) {
            emit_line(false);
            if (character == L' ') return;
        }
        if (line.runs.empty() || !(line.runs.back().style == style)) {
            line.runs.push_back({style, {}, 0.0});
        }
        line.runs.back().text.push_back(character);
        line.runs.back().width += width;
        line.width += width;
        line.largest_font = (std::max)(line.largest_font, pdf_font_size(style));
    };

    for (const Paragraph& paragraph : paragraphs) {
        line.alignment = paragraph.alignment;
        for (const TextRun& run : paragraph.runs) {
            for (const wchar_t character : run.text) {
                if (character == L'\f') {
                    emit_line(false);
                    begin_new_page();
                } else if (character == L'\r' || character == L'\n') {
                    emit_line(true);
                } else if (character == L'\t') {
                    for (int index = 0; index < 4; ++index)
                        add_character(L' ', run.style);
                } else if (character >= 0x20) {
                    add_character(character, run.style);
                }
            }
        }
        emit_line(true);
    }
    if (pages.empty()) pages.emplace_back();
    return pages;
}

bool write_pdf(const std::wstring& path,
               const std::vector<Paragraph>& paragraphs) {
    static constexpr std::array<const char*, 12> font_names = {
        "Helvetica", "Helvetica-Bold", "Helvetica-Oblique",
        "Helvetica-BoldOblique", "Times-Roman", "Times-Bold",
        "Times-Italic", "Times-BoldItalic", "Courier", "Courier-Bold",
        "Courier-Oblique", "Courier-BoldOblique"};
    const std::vector<std::string> contents = pdf_page_contents(paragraphs);
    constexpr int catalog_object = 1;
    constexpr int pages_object = 2;
    constexpr int first_font_object = 3;
    constexpr int first_page_object = first_font_object +
                                      static_cast<int>(font_names.size());
    const int object_count = first_page_object - 1 +
                             static_cast<int>(contents.size()) * 2;
    std::vector<std::string> objects(static_cast<std::size_t>(object_count + 1));
    objects[catalog_object] = "<< /Type /Catalog /Pages 2 0 R >>";

    std::string kids;
    for (std::size_t index = 0; index < contents.size(); ++index) {
        const int page_object = first_page_object + static_cast<int>(index) * 2;
        kids += std::to_string(page_object) + " 0 R ";
    }
    objects[pages_object] = "<< /Type /Pages /Kids [ " + kids +
        "] /Count " + std::to_string(contents.size()) + " >>";
    for (std::size_t index = 0; index < font_names.size(); ++index) {
        objects[first_font_object + static_cast<int>(index)] =
            "<< /Type /Font /Subtype /Type1 /BaseFont /" +
            std::string(font_names[index]) + " /Encoding /WinAnsiEncoding >>";
    }

    std::string resources = "<< /Font << ";
    for (std::size_t index = 0; index < font_names.size(); ++index) {
        resources += "/F" + std::to_string(index + 1) + " " +
                     std::to_string(first_font_object +
                                    static_cast<int>(index)) + " 0 R ";
    }
    resources += ">> >>";
    for (std::size_t index = 0; index < contents.size(); ++index) {
        const int page_object = first_page_object + static_cast<int>(index) * 2;
        const int content_object = page_object + 1;
        objects[page_object] =
            "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
            "/Resources " + resources + " /Contents " +
            std::to_string(content_object) + " 0 R >>";
        objects[content_object] =
            "<< /Length " + std::to_string(contents[index].size()) +
            " >>\nstream\n" + contents[index] + "endstream";
    }

    std::string pdf = "%PDF-1.4\n%\xe2\xe3\xcf\xd3\n";
    std::vector<std::size_t> offsets(static_cast<std::size_t>(object_count + 1));
    for (int object = 1; object <= object_count; ++object) {
        offsets[object] = pdf.size();
        pdf += std::to_string(object) + " 0 obj\n" + objects[object] +
               "\nendobj\n";
    }
    const std::size_t xref_offset = pdf.size();
    pdf += "xref\n0 " + std::to_string(object_count + 1) +
           "\n0000000000 65535 f \n";
    for (int object = 1; object <= object_count; ++object) {
        char entry[32]{};
        std::snprintf(entry, std::size(entry), "%010llu 00000 n \n",
                      static_cast<unsigned long long>(offsets[object]));
        pdf += entry;
    }
    pdf += "trailer\n<< /Size " + std::to_string(object_count + 1) +
           " /Root 1 0 R >>\nstartxref\n" + std::to_string(xref_offset) +
           "\n%%EOF\n";
    return write_bytes(path, pdf);
}

bool rtf_to_pdf(std::string_view rtf, const std::wstring& path) {
    RichEditDocument rich;
    return rich.load(rtf) &&
           write_pdf(path, paragraphs_from_rich_edit(rich));
}

int export_rtf_to_pdf_dialog(HWND owner, std::string_view rtf) {
    wchar_t path[32768] = L"Document.pdf";
    static const wchar_t filter[] = L"PDF Files (*.pdf)\0*.pdf\0\0";
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = owner;
    dialog.lpstrFilter = filter;
    dialog.lpstrFile = path;
    dialog.nMaxFile = static_cast<DWORD>(std::size(path));
    dialog.lpstrTitle = L"Export as PDF";
    dialog.lpstrDefExt = L"pdf";
    dialog.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_LONGNAMES |
                   OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    const DWORD test_path_length = GetEnvironmentVariableW(
        L"WORD1_TEST_PDF_PATH", path, static_cast<DWORD>(std::size(path)));
    const bool accepted =
        test_path_length > 0 && test_path_length < std::size(path) ?
            (SetEnvironmentVariableW(L"WORD1_TEST_PDF_PATH", nullptr), true) :
            GetSaveFileNameW(&dialog) != FALSE;
    if (!accepted) {
        return CommDlgExtendedError() == 0 ? -1 : false;
    }
    if (!rtf_to_pdf(rtf, path)) {
        MessageBoxW(owner,
            L"Word could not create the PDF. Check that the selected folder is writable.",
            L"Export as PDF", MB_OK | MB_ICONEXCLAMATION);
        return false;
    }
    return true;
}

}  // namespace

extern "C" int OpusModernPathIsDocx(const char* path) {
    return path != nullptr && has_extension(path, ".docx");
}

extern "C" int OpusModernDocxToRtfFile(const char* docx_path,
                                        const char* rtf_path) {
    std::string xml;
    return read_opc_part(wide_path(docx_path), L"/word/document.xml", xml) &&
           write_bytes(wide_path(rtf_path),
                        paragraphs_to_rtf(parse_document_xml(xml)));
}

extern "C" int OpusModernDocxToTextFile(const char* docx_path,
                                          const char* text_path) {
    std::string xml;
    return read_opc_part(wide_path(docx_path), L"/word/document.xml", xml) &&
           write_bytes(wide_path(text_path),
                       paragraphs_to_text(parse_document_xml(xml)));
}

extern "C" int OpusModernRtfFileToDocx(const char* rtf_path,
                                        const char* docx_path) {
    return rtf_to_docx(wide_path(rtf_path), wide_path(docx_path));
}

extern "C" int OpusModernRtfFileToPdf(const char* rtf_path,
                                       const char* pdf_path) {
    std::string rtf;
    return read_bytes(wide_path(rtf_path), rtf) &&
           rtf_to_pdf(rtf, wide_path(pdf_path));
}

extern "C" int OpusExportRtfToPdfDialog(HWND owner, const char* rtf) {
    return rtf == nullptr ? false :
        export_rtf_to_pdf_dialog(owner, rtf);
}

extern "C" int OpusExportTextToPdfDialog(HWND owner, const char* text,
                                           const int length) {
    if (text == nullptr || length < 0) return false;
    return export_rtf_to_pdf_dialog(
        owner, ansi_text_to_rtf(std::string_view(
                   text, static_cast<std::size_t>(length))));
}
