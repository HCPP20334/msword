#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <msopc.h>
#include <richedit.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cwctype>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <iterator>
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

std::wstring ansi_to_wide(std::string_view value) {
    if (value.empty()) return {};
    const int count = MultiByteToWideChar(
        1252, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(count > 0 ? static_cast<std::size_t>(count) : 0,
                        L'\0');
    if (count > 0) {
        MultiByteToWideChar(1252, 0, value.data(),
                            static_cast<int>(value.size()), result.data(),
                            count);
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
    bool strike = false;
    bool small_caps = false;
    bool all_caps = false;
    bool hidden = false;
    int half_points = 20;
    COLORREF color = RGB(0, 0, 0);
    bool auto_color = true;
    std::wstring font = L"Arial";
    bool operator==(const RunStyle&) const = default;
};

struct TextRun { RunStyle style; std::wstring text; };
struct DocumentSettings {
    bool valid = false;
    int page_width = 0;
    int page_height = 0;
    int margin_left = 0;
    int margin_right = 0;
    int margin_top = 0;
    int margin_bottom = 0;
};
struct Paragraph {
    int alignment = PFA_LEFT;
    int left_indent = 0;
    int right_indent = 0;
    int first_line_indent = 0;
    int space_before = 0;
    int space_after = 0;
    int line_spacing = 0;
    bool keep_together = false;
    bool keep_with_next = false;
    bool page_break_before = false;
    bool bottom_border = false;
    std::vector<TextRun> runs;
};

std::string_view element_block(std::string_view xml,
                               std::string_view local_name) {
    const std::string opening = "<w:" + std::string(local_name);
    std::size_t start = 0;
    while ((start = xml.find(opening, start)) != std::string_view::npos) {
        const std::size_t boundary = start + opening.size();
        if (boundary < xml.size() &&
            (xml[boundary] == '>' || xml[boundary] == '/' ||
             std::isspace(static_cast<unsigned char>(xml[boundary])))) {
            const std::size_t open_end = xml.find('>', boundary);
            if (open_end == std::string_view::npos) return {};
            if (open_end > start && xml[open_end - 1] == '/')
                return xml.substr(start, open_end - start + 1);
            const std::string closing = "</w:" + std::string(local_name) + ">";
            const std::size_t close = xml.find(closing, open_end + 1);
            if (close == std::string_view::npos) return {};
            return xml.substr(start, close + closing.size() - start);
        }
        start = boundary;
    }
    return {};
}

int property_state(std::string_view properties, const char* local_name) {
    std::size_t position = 0;
    while ((position = properties.find('<', position)) != std::string_view::npos) {
        const std::size_t end = properties.find('>', position + 1);
        if (end == std::string_view::npos) break;
        const std::string_view tag = properties.substr(position, end - position + 1);
        if (local_tag_name(tag) == local_name) {
            const std::string value = tag_attribute(tag, "val");
            return value == "0" || value == "false" || value == "none" ? 0 : 1;
        }
        position = end + 1;
    }
    return -1;
}

bool property_enabled(std::string_view properties, const char* local_name) {
    return property_state(properties, local_name) == 1;
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

void apply_run_properties(RunStyle& style, std::string_view props) {
    const auto apply_switch = [&](const char* name, bool& target) {
        const int state = property_state(props, name);
        if (state >= 0) target = state != 0;
    };
    apply_switch("b", style.bold);
    apply_switch("i", style.italic);
    apply_switch("u", style.underline);
    apply_switch("strike", style.strike);
    apply_switch("smallCaps", style.small_caps);
    apply_switch("caps", style.all_caps);
    apply_switch("vanish", style.hidden);
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
    } else if (first_property_value(props, "color") == "auto") {
        style.auto_color = true;
        style.color = RGB(0, 0, 0);
    }
}

void apply_paragraph_properties(Paragraph& paragraph, std::string_view props) {
    if (const std::string alignment = first_property_value(props, "jc");
        !alignment.empty()) {
        if (alignment == "center") paragraph.alignment = PFA_CENTER;
        else if (alignment == "right") paragraph.alignment = PFA_RIGHT;
        else if (alignment == "both" || alignment == "distribute")
            paragraph.alignment = PFA_JUSTIFY;
        else paragraph.alignment = PFA_LEFT;
    }
    if (const std::string_view indent = element_block(props, "ind"); !indent.empty()) {
        const std::string left = tag_attribute(indent, "left");
        const std::string right = tag_attribute(indent, "right");
        const std::string first = tag_attribute(indent, "firstLine");
        const std::string hanging = tag_attribute(indent, "hanging");
        if (!left.empty()) paragraph.left_indent = std::atoi(left.c_str());
        if (!right.empty()) paragraph.right_indent = std::atoi(right.c_str());
        if (!first.empty()) paragraph.first_line_indent = std::atoi(first.c_str());
        if (!hanging.empty()) paragraph.first_line_indent = -std::atoi(hanging.c_str());
    }
    if (const std::string_view spacing = element_block(props, "spacing");
        !spacing.empty()) {
        const std::string before = tag_attribute(spacing, "before");
        const std::string after = tag_attribute(spacing, "after");
        const std::string line = tag_attribute(spacing, "line");
        if (!before.empty()) paragraph.space_before = std::atoi(before.c_str());
        if (!after.empty()) paragraph.space_after = std::atoi(after.c_str());
        if (!line.empty()) paragraph.line_spacing = std::atoi(line.c_str());
    }
    const auto apply_switch = [&](const char* name, bool& target) {
        const int state = property_state(props, name);
        if (state >= 0) target = state != 0;
    };
    apply_switch("keepLines", paragraph.keep_together);
    apply_switch("keepNext", paragraph.keep_with_next);
    apply_switch("pageBreakBefore", paragraph.page_break_before);
    if (const std::string_view borders = element_block(props, "pBdr");
        !borders.empty()) {
        const std::string_view bottom = element_block(borders, "bottom");
        const std::string value = tag_attribute(bottom, "val");
        paragraph.bottom_border = !bottom.empty() && value != "nil" &&
                                  value != "none";
    }
}

struct StyleDefinition {
    std::string based_on;
    std::string run_properties;
    std::string paragraph_properties;
};

struct StyleCatalog {
    RunStyle default_run;
    Paragraph default_paragraph;
    std::map<std::string, StyleDefinition> definitions;
};

StyleCatalog parse_style_catalog(std::string_view xml) {
    StyleCatalog catalog;
    if (const std::string_view defaults = element_block(xml, "docDefaults");
        !defaults.empty()) {
        const std::string_view run_default = element_block(defaults, "rPrDefault");
        apply_run_properties(catalog.default_run,
                             element_block(run_default, "rPr"));
        const std::string_view paragraph_default =
            element_block(defaults, "pPrDefault");
        apply_paragraph_properties(catalog.default_paragraph,
                                   element_block(paragraph_default, "pPr"));
    }
    std::size_t position = 0;
    while ((position = xml.find("<w:style", position)) != std::string_view::npos) {
        const std::size_t boundary = position + 8;
        if (boundary >= xml.size() ||
            (!std::isspace(static_cast<unsigned char>(xml[boundary])) &&
             xml[boundary] != '>')) {
            position = boundary;
            continue;
        }
        const std::size_t open_end = xml.find('>', boundary);
        const std::size_t close = xml.find("</w:style>", open_end);
        if (open_end == std::string_view::npos || close == std::string_view::npos)
            break;
        const std::string_view opening = xml.substr(position, open_end - position + 1);
        const std::string id = tag_attribute(opening, "styleId");
        const std::string_view block = xml.substr(position, close + 10 - position);
        if (!id.empty()) {
            StyleDefinition definition;
            definition.based_on = first_property_value(block, "basedOn");
            if (const std::string_view props = element_block(block, "rPr");
                !props.empty()) definition.run_properties.assign(props);
            if (const std::string_view props = element_block(block, "pPr");
                !props.empty()) definition.paragraph_properties.assign(props);
            catalog.definitions[id] = std::move(definition);
        }
        position = close + 10;
    }
    return catalog;
}

void apply_style(const StyleCatalog& catalog, const std::string& id,
                 RunStyle& run, Paragraph& paragraph, const int depth = 0) {
    if (id.empty() || depth > 16) return;
    const auto found = catalog.definitions.find(id);
    if (found == catalog.definitions.end()) return;
    if (!found->second.based_on.empty() && found->second.based_on != id) {
        apply_style(catalog, found->second.based_on, run, paragraph, depth + 1);
    }
    apply_run_properties(run, found->second.run_properties);
    apply_paragraph_properties(paragraph, found->second.paragraph_properties);
}

RunStyle parse_run_style(std::string_view run, RunStyle style,
                         const StyleCatalog& catalog) {
    const std::string_view props = element_block(run, "rPr");
    if (const std::string character_style =
            first_property_value(props, "rStyle"); !character_style.empty()) {
        Paragraph ignored;
        apply_style(catalog, character_style, style, ignored);
    }
    apply_run_properties(style, props);
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
            else if (name == "br") {
                text.push_back(tag_attribute(tag, "type") == "page" ?
                                   L'\f' : L'\n');
            } else if (name == "cr") text.push_back(L'\n');
            else if (name == "noBreakHyphen") text.push_back(L'-');
            else if (name == "softHyphen") text.push_back(L'\x00ad');
            position = tag_end + 1;
        }
    }
    return text;
}

std::vector<Paragraph> parse_paragraphs_flat(
    std::string_view xml, const StyleCatalog& catalog = {}) {
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
        Paragraph paragraph = catalog.default_paragraph;
        RunStyle paragraph_run = catalog.default_run;
        apply_style(catalog, "Normal", paragraph_run, paragraph);
        const std::string_view paragraph_properties = element_block(block, "pPr");
        if (const std::string paragraph_style =
                first_property_value(paragraph_properties, "pStyle");
            !paragraph_style.empty() && paragraph_style != "Normal") {
            apply_style(catalog, paragraph_style, paragraph_run, paragraph);
        }
        apply_paragraph_properties(paragraph, paragraph_properties);

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
            if (!text.empty()) paragraph.runs.push_back(
                {parse_run_style(run, paragraph_run, catalog), std::move(text)});
            run_position = run_close + 6;
        }
        paragraphs.push_back(std::move(paragraph));
        position = close + 6;
    }
    if (paragraphs.empty()) paragraphs.push_back({});
    return paragraphs;
}

struct TableLayout {
    std::size_t first_paragraph = 0;
    std::size_t paragraph_count = 0;
    int rows = 0;
    int columns = 0;
};

std::size_t find_word_tag(std::string_view xml, std::string_view name,
                          std::size_t position) {
    const std::string opening = "<w:" + std::string(name);
    while ((position = xml.find(opening, position)) != std::string_view::npos) {
        const std::size_t boundary = position + opening.size();
        if (boundary < xml.size() &&
            (xml[boundary] == '>' || xml[boundary] == '/' ||
             std::isspace(static_cast<unsigned char>(xml[boundary])))) {
            return position;
        }
        position = boundary;
    }
    return std::string_view::npos;
}

std::vector<Paragraph> parse_document_xml(
    std::string_view xml, const StyleCatalog& catalog = {},
    std::vector<TableLayout>* table_layouts = nullptr) {
    std::vector<Paragraph> paragraphs;
    if (table_layouts != nullptr) table_layouts->clear();
    std::size_t cursor = 0;
    while (cursor < xml.size()) {
        const std::size_t table_start = find_word_tag(xml, "tbl", cursor);
        if (table_start == std::string_view::npos) {
            const std::string_view remainder = xml.substr(cursor);
            if (find_word_tag(remainder, "p", 0) != std::string_view::npos) {
                std::vector<Paragraph> tail =
                    parse_paragraphs_flat(remainder, catalog);
                paragraphs.insert(paragraphs.end(),
                                  std::make_move_iterator(tail.begin()),
                                  std::make_move_iterator(tail.end()));
            }
            break;
        }
        const std::string_view prefix =
            xml.substr(cursor, table_start - cursor);
        if (find_word_tag(prefix, "p", 0) != std::string_view::npos) {
            std::vector<Paragraph> before =
                parse_paragraphs_flat(prefix, catalog);
            paragraphs.insert(paragraphs.end(),
                              std::make_move_iterator(before.begin()),
                              std::make_move_iterator(before.end()));
        }
        const std::size_t table_close = xml.find("</w:tbl>", table_start);
        if (table_close == std::string_view::npos) break;
        const std::size_t table_end = table_close + std::strlen("</w:tbl>");
        const std::string_view table =
            xml.substr(table_start, table_end - table_start);
        TableLayout layout;
        layout.first_paragraph = paragraphs.size();
        std::size_t row_cursor = 0;
        while (true) {
            const std::size_t row_start = find_word_tag(table, "tr", row_cursor);
            if (row_start == std::string_view::npos) break;
            const std::size_t row_close = table.find("</w:tr>", row_start);
            if (row_close == std::string_view::npos) break;
            const std::size_t row_end = row_close + std::strlen("</w:tr>");
            const std::string_view row = table.substr(row_start,
                                                       row_end - row_start);
            std::vector<std::vector<Paragraph>> cells;
            std::size_t cell_cursor = 0;
            while (true) {
                const std::size_t cell_start = find_word_tag(row, "tc", cell_cursor);
                if (cell_start == std::string_view::npos) break;
                const std::size_t cell_close = row.find("</w:tc>", cell_start);
                if (cell_close == std::string_view::npos) break;
                const std::size_t cell_end = cell_close + std::strlen("</w:tc>");
                cells.push_back(parse_paragraphs_flat(
                    row.substr(cell_start, cell_end - cell_start), catalog));
                cell_cursor = cell_end;
            }
            std::size_t line_count = 0;
            for (const auto& cell : cells)
                line_count = (std::max)(line_count, cell.size());
            if (!cells.empty()) {
                layout.columns = (std::max)(layout.columns,
                                            static_cast<int>(cells.size()));
                for (std::size_t line = 0; line < line_count; ++line) {
                    for (auto& cell : cells) {
                        if (line < cell.size())
                            paragraphs.push_back(std::move(cell[line]));
                        else
                            paragraphs.push_back({});
                    }
                    ++layout.rows;
                }
            }
            row_cursor = row_end;
        }
        layout.paragraph_count = paragraphs.size() - layout.first_paragraph;
        if (layout.rows > 0 && layout.columns > 1 &&
            layout.paragraph_count ==
                static_cast<std::size_t>(layout.rows * layout.columns)) {
            if (table_layouts != nullptr) table_layouts->push_back(layout);
        } else {
            paragraphs.resize(layout.first_paragraph);
            std::vector<Paragraph> flat = parse_paragraphs_flat(table, catalog);
            paragraphs.insert(paragraphs.end(),
                              std::make_move_iterator(flat.begin()),
                              std::make_move_iterator(flat.end()));
        }
        cursor = table_end;
    }
    if (paragraphs.empty()) paragraphs.push_back({});
    return paragraphs;
}

DocumentSettings parse_document_settings(std::string_view xml) {
    DocumentSettings settings;
    const std::string_view section = element_block(xml, "sectPr");
    const std::string_view size = element_block(section, "pgSz");
    const std::string_view margins = element_block(section, "pgMar");
    const auto integer_attribute = [](std::string_view tag,
                                      const char* name) {
        const std::string value = tag_attribute(tag, name);
        return value.empty() ? 0 : std::atoi(value.c_str());
    };
    settings.page_width = integer_attribute(size, "w");
    settings.page_height = integer_attribute(size, "h");
    settings.margin_left = integer_attribute(margins, "left");
    settings.margin_right = integer_attribute(margins, "right");
    settings.margin_top = integer_attribute(margins, "top");
    settings.margin_bottom = integer_attribute(margins, "bottom");
    settings.valid = settings.page_width > 0 && settings.page_height > 0 &&
                     settings.margin_left >= 0 && settings.margin_right >= 0 &&
                     settings.margin_top >= 0 && settings.margin_bottom >= 0;
    return settings;
}

struct PendingRunFormat {
    long cp_first = 0;
    long cp_lim = 0;
    RunStyle style;
};

struct PendingParagraphFormat {
    long cp_first = 0;
    long cp_lim = 0;
    Paragraph paragraph;
};

struct PendingTableFormat {
    long cp_first = 0;
    long cp_lim = 0;
    int first_paragraph = 0;
    int rows = 0;
    int columns = 0;
};

struct PendingDocxImport {
    std::vector<PendingRunFormat> runs;
    std::vector<PendingParagraphFormat> paragraphs;
    std::vector<PendingTableFormat> tables;
    DocumentSettings settings;
};

PendingDocxImport pending_docx_import;

struct PendingPdfExport {
    std::vector<Paragraph> paragraphs;
    DocumentSettings settings;
};

PendingPdfExport pending_pdf_export;

std::string ansi_run_text(std::wstring_view text) {
    std::string result;
    for (const wchar_t character : text) {
        if (character == L'\n') {
            result += "\r\n";
        } else if (character == L'\r') {
            if (result.empty() || result.back() != '\r') result.push_back('\r');
        } else {
            result += wide_to_ansi(std::wstring_view(&character, 1));
        }
    }
    return result;
}

std::string paragraphs_to_text(const std::vector<Paragraph>& paragraphs,
                               PendingDocxImport* pending = nullptr,
                               const std::vector<TableLayout>* tables = nullptr) {
    std::string text;
    if (pending != nullptr) *pending = {};
    for (std::size_t index = 0; index < paragraphs.size(); ++index) {
        PendingParagraphFormat paragraph_format;
        paragraph_format.cp_first = static_cast<long>(text.size());
        paragraph_format.paragraph = paragraphs[index];
        paragraph_format.paragraph.runs.clear();
        for (const auto& run : paragraphs[index].runs) {
            PendingRunFormat run_format;
            run_format.cp_first = static_cast<long>(text.size());
            run_format.style = run.style;
            text += ansi_run_text(run.text);
            run_format.cp_lim = static_cast<long>(text.size());
            if (pending != nullptr && run_format.cp_lim > run_format.cp_first)
                pending->runs.push_back(std::move(run_format));
        }
        if (index + 1 < paragraphs.size()) text += "\r\n";
        paragraph_format.cp_lim = static_cast<long>(text.size());
        if (pending != nullptr)
            pending->paragraphs.push_back(std::move(paragraph_format));
    }
    if (pending != nullptr && tables != nullptr) {
        for (const TableLayout& table : *tables) {
            if (table.paragraph_count == 0 ||
                table.first_paragraph >= pending->paragraphs.size() ||
                table.first_paragraph + table.paragraph_count >
                    pending->paragraphs.size()) continue;
            PendingTableFormat format;
            format.cp_first =
                pending->paragraphs[table.first_paragraph].cp_first;
            format.cp_lim = pending->paragraphs[
                table.first_paragraph + table.paragraph_count - 1].cp_lim;
            format.first_paragraph = static_cast<int>(table.first_paragraph);
            format.rows = table.rows;
            format.columns = table.columns;
            pending->tables.push_back(format);
        }
    }
    return text;
}

bool load_docx_paragraphs(const char* path,
                          std::vector<Paragraph>& paragraphs,
                          DocumentSettings* settings = nullptr,
                          std::vector<TableLayout>* tables = nullptr) {
    std::string document;
    std::string styles;
    if (!read_opc_part(wide_path(path), L"/word/document.xml", document))
        return false;
    read_opc_part(wide_path(path), L"/word/styles.xml", styles);
    paragraphs = parse_document_xml(document, parse_style_catalog(styles),
                                    tables);
    if (settings != nullptr) *settings = parse_document_settings(document);
    return true;
}

int legacy_color_index(const RunStyle& style) {
    if (style.auto_color) return 0;
    const int red = GetRValue(style.color);
    const int green = GetGValue(style.color);
    const int blue = GetBValue(style.color);
    const int maximum = (std::max)({red, green, blue});
    const int minimum = (std::min)({red, green, blue});
    if (maximum < 64 || maximum - minimum < 32) {
        return maximum > 208 ? 8 : 1;
    }
    const bool high_red = red * 5 >= maximum * 3;
    const bool high_green = green * 5 >= maximum * 3;
    const bool high_blue = blue * 5 >= maximum * 3;
    if (high_red && high_green && !high_blue) return 7;
    if (high_red && high_blue && !high_green) return 5;
    if (high_green && high_blue && !high_red) return 3;
    if (maximum == red) return 6;
    if (maximum == green) return 4;
    return 2;
}

void apply_legacy_color(const int color_index, RunStyle& style) {
    static constexpr std::array<COLORREF, 9> colors = {
        RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 255),
        RGB(0, 255, 255), RGB(0, 128, 0), RGB(255, 0, 255),
        RGB(255, 0, 0), RGB(255, 255, 0), RGB(255, 255, 255)};
    style.auto_color = color_index <= 0 ||
        color_index >= static_cast<int>(colors.size());
    style.color = style.auto_color ? RGB(0, 0, 0) : colors[color_index];
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
    double largest_font = 0.0;
    double left = 72.0;
    double writable_width = 468.0;
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
    double x = line.left;
    if (line.alignment == PFA_CENTER) {
        x += ((std::max)(0.0, line.writable_width - line.width)) / 2.0;
    } else if (line.alignment == PFA_RIGHT) {
        x += (std::max)(0.0, line.writable_width - line.width);
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
    const std::vector<Paragraph>& paragraphs,
    const DocumentSettings& settings) {
    const double page_width = settings.valid ?
        static_cast<double>(settings.page_width) / 20.0 : 612.0;
    const double page_height = settings.valid ?
        static_cast<double>(settings.page_height) / 20.0 : 792.0;
    const double margin_left = settings.valid ?
        static_cast<double>(settings.margin_left) / 20.0 : 72.0;
    const double margin_right = settings.valid ?
        static_cast<double>(settings.margin_right) / 20.0 : 72.0;
    const double margin_top = settings.valid ?
        static_cast<double>(settings.margin_top) / 20.0 : 72.0;
    const double margin_bottom = settings.valid ?
        static_cast<double>(settings.margin_bottom) / 20.0 : 72.0;
    const double top = (std::max)(margin_bottom + 36.0,
                                  page_height - margin_top);
    const double bottom = (std::max)(18.0, margin_bottom);
    std::vector<std::string> pages(1);
    double cursor = top;

    const auto begin_new_page = [&]() {
        pages.emplace_back();
        cursor = top;
    };

    bool first_paragraph = true;
    for (const Paragraph& paragraph : paragraphs) {
        if (paragraph.page_break_before && !first_paragraph) begin_new_page();
        first_paragraph = false;
        cursor -= (std::max)(0.0,
            static_cast<double>(paragraph.space_before) / 20.0);
        if (cursor <= bottom + 12.0) begin_new_page();

        PdfLine line;
        bool first_visual_line = true;
        const auto reset_line = [&]() {
            const double first_indent = first_visual_line ?
                static_cast<double>(paragraph.first_line_indent) / 20.0 : 0.0;
            line.alignment = paragraph.alignment;
            line.left = margin_left +
                static_cast<double>(paragraph.left_indent) / 20.0 +
                first_indent;
            const double right = page_width - margin_right -
                static_cast<double>(paragraph.right_indent) / 20.0;
            line.writable_width = (std::max)(36.0, right - line.left);
        };
        reset_line();
        const auto emit_line = [&](const bool force) {
            if (!force && line.runs.empty()) return;
            const double largest_font = line.largest_font > 0.0 ?
                line.largest_font : 12.0;
            double line_height = (std::max)(largest_font * 1.20, 4.8);
            if (paragraph.line_spacing < 0) {
                line_height = (std::max)(4.8,
                    -static_cast<double>(paragraph.line_spacing) / 20.0);
            } else if (paragraph.line_spacing > 0) {
                line_height = (std::max)(line_height,
                    static_cast<double>(paragraph.line_spacing) / 20.0);
            }
            if (cursor - line_height < bottom) begin_new_page();
            const double baseline = cursor - largest_font;
            append_pdf_line(pages.back(), line, baseline);
            cursor -= line_height;
            first_visual_line = false;
            line = PdfLine{};
            reset_line();
        };
        const auto add_character = [&](const wchar_t character,
                                       const RunStyle& style) {
            const double width = pdf_character_width(character, style);
            if (!line.runs.empty() &&
                line.width + width > line.writable_width) {
                emit_line(false);
                if (character == L' ') return;
            }
            if (line.runs.empty() || !(line.runs.back().style == style)) {
                line.runs.push_back({style, {}, 0.0});
            }
            line.runs.back().text.push_back(character);
            line.runs.back().width += width;
            line.width += width;
            line.largest_font =
                (std::max)(line.largest_font, pdf_font_size(style));
        };
        const auto add_word = [&](std::wstring_view word,
                                  const RunStyle& style) {
            double word_width = 0.0;
            for (const wchar_t character : word)
                word_width += pdf_character_width(character, style);
            if (!line.runs.empty() &&
                line.width + word_width > line.writable_width) {
                emit_line(false);
            }
            for (const wchar_t character : word)
                add_character(character, style);
        };

        for (const TextRun& run : paragraph.runs) {
            if (run.style.hidden) continue;
            for (std::size_t position = 0; position < run.text.size();) {
                const wchar_t character = run.text[position];
                if (character == L'\f') {
                    emit_line(false);
                    begin_new_page();
                } else if (character == L'\r' || character == L'\n') {
                    emit_line(true);
                } else if (character == L'\t') {
                    for (int index = 0; index < 4; ++index)
                        add_character(L' ', run.style);
                } else if (character == L' ') {
                    if (!line.runs.empty()) add_character(character, run.style);
                } else if (character >= 0x20) {
                    std::size_t end = position + 1;
                    while (end < run.text.size() && run.text[end] > L' ' &&
                           run.text[end] != L'\f') ++end;
                    add_word(std::wstring_view(run.text).substr(
                                 position, end - position), run.style);
                    position = end;
                    continue;
                }
                ++position;
            }
        }
        emit_line(true);
        if (paragraph.bottom_border) {
            const double border_left = margin_left +
                static_cast<double>(paragraph.left_indent) / 20.0;
            const double border_right = page_width - margin_right -
                static_cast<double>(paragraph.right_indent) / 20.0;
            pages.back() += "q\n0 0 0 RG\n0.5 w\n" +
                pdf_number(border_left) + " " + pdf_number(cursor + 2.0) +
                " m\n" + pdf_number(border_right) + " " +
                pdf_number(cursor + 2.0) + " l\nS\nQ\n";
        }
        cursor -= (std::max)(0.0,
            static_cast<double>(paragraph.space_after) / 20.0);
    }
    if (pages.empty()) pages.emplace_back();
    return pages;
}

bool write_pdf(const std::wstring& path,
               const std::vector<Paragraph>& paragraphs,
               const DocumentSettings& settings = {}) {
    static constexpr std::array<const char*, 12> font_names = {
        "Helvetica", "Helvetica-Bold", "Helvetica-Oblique",
        "Helvetica-BoldOblique", "Times-Roman", "Times-Bold",
        "Times-Italic", "Times-BoldItalic", "Courier", "Courier-Bold",
        "Courier-Oblique", "Courier-BoldOblique"};
    const std::vector<std::string> contents =
        pdf_page_contents(paragraphs, settings);
    const double page_width = settings.valid ?
        static_cast<double>(settings.page_width) / 20.0 : 612.0;
    const double page_height = settings.valid ?
        static_cast<double>(settings.page_height) / 20.0 : 792.0;
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
            "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 " +
            pdf_number(page_width) + " " + pdf_number(page_height) + "] "
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

int export_paragraphs_to_pdf_dialog(
    HWND owner, const std::vector<Paragraph>& paragraphs,
    const DocumentSettings& settings = {}) {
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
    if (!write_pdf(path, paragraphs, settings)) {
        MessageBoxW(owner,
            L"Word could not create the PDF. Check that the selected folder is writable.",
            L"Export as PDF", MB_OK | MB_ICONEXCLAMATION);
        return false;
    }
    return true;
}

int export_rtf_to_pdf_dialog(HWND owner, std::string_view rtf) {
    RichEditDocument rich;
    if (!rich.load(rtf)) return false;
    return export_paragraphs_to_pdf_dialog(
        owner, paragraphs_from_rich_edit(rich));
}

}  // namespace

extern "C" int OpusModernPathIsDocx(const char* path) {
    return path != nullptr && has_extension(path, ".docx");
}

extern "C" int OpusModernDocxToRtfFile(const char* docx_path,
                                         const char* rtf_path) {
    std::vector<Paragraph> paragraphs;
    return load_docx_paragraphs(docx_path, paragraphs) &&
           write_bytes(wide_path(rtf_path), paragraphs_to_rtf(paragraphs));
}

extern "C" int OpusModernDocxToTextFile(const char* docx_path,
                                           const char* text_path) {
    std::vector<Paragraph> paragraphs;
    std::vector<TableLayout> tables;
    DocumentSettings settings;
    if (!load_docx_paragraphs(docx_path, paragraphs, &settings, &tables)) {
        pending_docx_import = {};
        return false;
    }
    const std::string text = paragraphs_to_text(paragraphs,
                                                &pending_docx_import, &tables);
    pending_docx_import.settings = settings;
    if (!write_bytes(wide_path(text_path), text)) {
        pending_docx_import = {};
        return false;
    }
    return true;
}

extern "C" int OpusModernPendingDocxRunCount() {
    return static_cast<int>(pending_docx_import.runs.size());
}

extern "C" int OpusModernPendingDocxParagraphCount() {
    return static_cast<int>(pending_docx_import.paragraphs.size());
}

extern "C" int OpusModernPendingDocxTableCount() {
    return static_cast<int>(pending_docx_import.tables.size());
}

extern "C" int OpusModernGetPendingDocxTable(
    const int index, long* cp_first, long* cp_lim, int* first_paragraph,
    int* rows, int* columns) {
    if (index < 0 || static_cast<std::size_t>(index) >=
                         pending_docx_import.tables.size()) return false;
    const PendingTableFormat& record = pending_docx_import.tables[index];
    if (cp_first != nullptr) *cp_first = record.cp_first;
    if (cp_lim != nullptr) *cp_lim = record.cp_lim;
    if (first_paragraph != nullptr)
        *first_paragraph = record.first_paragraph;
    if (rows != nullptr) *rows = record.rows;
    if (columns != nullptr) *columns = record.columns;
    return true;
}

extern "C" int OpusModernGetPendingDocxParagraphRange(
    const int index, long* cp_first, long* cp_lim) {
    if (index < 0 || static_cast<std::size_t>(index) >=
                         pending_docx_import.paragraphs.size()) return false;
    const PendingParagraphFormat& record =
        pending_docx_import.paragraphs[index];
    if (cp_first != nullptr) *cp_first = record.cp_first;
    if (cp_lim != nullptr) *cp_lim = record.cp_lim;
    return true;
}

extern "C" int OpusModernGetPendingDocxPage(
    int* page_width, int* page_height, int* margin_left, int* margin_right,
    int* margin_top, int* margin_bottom) {
    const DocumentSettings& settings = pending_docx_import.settings;
    if (!settings.valid) return false;
    if (page_width != nullptr) *page_width = settings.page_width;
    if (page_height != nullptr) *page_height = settings.page_height;
    if (margin_left != nullptr) *margin_left = settings.margin_left;
    if (margin_right != nullptr) *margin_right = settings.margin_right;
    if (margin_top != nullptr) *margin_top = settings.margin_top;
    if (margin_bottom != nullptr) *margin_bottom = settings.margin_bottom;
    return true;
}

extern "C" int OpusModernGetPendingDocxRun(
    const int index, long* cp_first, long* cp_lim, int* bold, int* italic,
    int* underline, int* strike, int* small_caps, int* all_caps, int* hidden,
    int* half_points, int* color_index, char* font, const int font_capacity) {
    if (index < 0 || static_cast<std::size_t>(index) >=
                         pending_docx_import.runs.size()) return false;
    const PendingRunFormat& record = pending_docx_import.runs[index];
    if (cp_first != nullptr) *cp_first = record.cp_first;
    if (cp_lim != nullptr) *cp_lim = record.cp_lim;
    if (bold != nullptr) *bold = record.style.bold;
    if (italic != nullptr) *italic = record.style.italic;
    if (underline != nullptr) *underline = record.style.underline;
    if (strike != nullptr) *strike = record.style.strike;
    if (small_caps != nullptr) *small_caps = record.style.small_caps;
    if (all_caps != nullptr) *all_caps = record.style.all_caps;
    if (hidden != nullptr) *hidden = record.style.hidden;
    if (half_points != nullptr) *half_points = record.style.half_points;
    if (color_index != nullptr) *color_index = legacy_color_index(record.style);
    if (font != nullptr && font_capacity > 0) {
        const std::string ansi_font = wide_to_ansi(record.style.font);
        lstrcpynA(font, ansi_font.c_str(), font_capacity);
    }
    return true;
}

extern "C" int OpusModernGetPendingDocxParagraph(
    const int index, long* cp_first, long* cp_lim, int* alignment,
    int* left_indent, int* right_indent, int* first_line_indent,
    int* space_before, int* space_after, int* line_spacing,
    int* keep_together, int* keep_with_next, int* page_break_before,
    int* bottom_border) {
    if (index < 0 || static_cast<std::size_t>(index) >=
                         pending_docx_import.paragraphs.size()) return false;
    const PendingParagraphFormat& record =
        pending_docx_import.paragraphs[index];
    if (cp_first != nullptr) *cp_first = record.cp_first;
    if (cp_lim != nullptr) *cp_lim = record.cp_lim;
    if (alignment != nullptr) {
        *alignment = record.paragraph.alignment == PFA_CENTER ? 1 :
            record.paragraph.alignment == PFA_RIGHT ? 2 :
            record.paragraph.alignment == PFA_JUSTIFY ? 3 : 0;
    }
    if (left_indent != nullptr) *left_indent = record.paragraph.left_indent;
    if (right_indent != nullptr) *right_indent = record.paragraph.right_indent;
    if (first_line_indent != nullptr)
        *first_line_indent = record.paragraph.first_line_indent;
    if (space_before != nullptr) *space_before = record.paragraph.space_before;
    if (space_after != nullptr) *space_after = record.paragraph.space_after;
    if (line_spacing != nullptr) *line_spacing = record.paragraph.line_spacing;
    if (keep_together != nullptr) *keep_together = record.paragraph.keep_together;
    if (keep_with_next != nullptr) *keep_with_next = record.paragraph.keep_with_next;
    if (page_break_before != nullptr)
        *page_break_before = record.paragraph.page_break_before;
    if (bottom_border != nullptr) *bottom_border = record.paragraph.bottom_border;
    return true;
}

extern "C" void OpusModernClearPendingDocxFormatting() {
    pending_docx_import = {};
}

extern "C" int OpusPdfSnapshotBegin(
    const int page_width, const int page_height, const int margin_left,
    const int margin_right, const int margin_top, const int margin_bottom) {
    pending_pdf_export = {};
    DocumentSettings& settings = pending_pdf_export.settings;
    settings.page_width = page_width;
    settings.page_height = page_height;
    settings.margin_left = (std::max)(0, margin_left);
    settings.margin_right = (std::max)(0, margin_right);
    settings.margin_top = (std::max)(0, margin_top);
    settings.margin_bottom = (std::max)(0, margin_bottom);
    settings.valid = page_width > 0 && page_height > 0 &&
        settings.margin_left + settings.margin_right < page_width &&
        settings.margin_top + settings.margin_bottom < page_height;
    return true;
}

extern "C" int OpusPdfSnapshotAddParagraph(
    const int alignment, const int left_indent, const int right_indent,
    const int first_line_indent, const int space_before,
    const int space_after, const int line_spacing,
    const int keep_together, const int keep_with_next,
    const int page_break_before, const int bottom_border) {
    Paragraph paragraph;
    paragraph.alignment = alignment == 1 ? PFA_CENTER :
        alignment == 2 ? PFA_RIGHT :
        alignment == 3 ? PFA_JUSTIFY : PFA_LEFT;
    paragraph.left_indent = left_indent;
    paragraph.right_indent = right_indent;
    paragraph.first_line_indent = first_line_indent;
    paragraph.space_before = space_before;
    paragraph.space_after = space_after;
    paragraph.line_spacing = line_spacing;
    paragraph.keep_together = keep_together != 0;
    paragraph.keep_with_next = keep_with_next != 0;
    paragraph.page_break_before = page_break_before != 0;
    paragraph.bottom_border = bottom_border != 0;
    pending_pdf_export.paragraphs.push_back(std::move(paragraph));
    return true;
}

extern "C" int OpusPdfSnapshotAddRun(
    const char* text, const int length, const char* font,
    const int half_points, const int bold, const int italic,
    const int underline, const int strike, const int small_caps,
    const int all_caps, const int hidden, const int color_index) {
    if (text == nullptr || length < 0 ||
        pending_pdf_export.paragraphs.empty()) return false;
    RunStyle style;
    style.bold = bold != 0;
    style.italic = italic != 0;
    style.underline = underline != 0;
    style.strike = strike != 0;
    style.small_caps = small_caps != 0;
    style.all_caps = all_caps != 0;
    style.hidden = hidden != 0;
    style.half_points = half_points >= 8 ? half_points : 20;
    if (font != nullptr && *font != '\0') style.font = ansi_to_wide(font);
    apply_legacy_color(color_index, style);
    std::wstring run_text = ansi_to_wide(
        std::string_view(text, static_cast<std::size_t>(length)));
    if (style.all_caps || style.small_caps) {
        for (wchar_t& character : run_text) {
            character = static_cast<wchar_t>(std::towupper(character));
        }
    }
    if (style.small_caps && !style.all_caps) {
        style.half_points = (std::max)(8, style.half_points * 4 / 5);
    }
    pending_pdf_export.paragraphs.back().runs.push_back(
        {style, std::move(run_text)});
    return true;
}

extern "C" int OpusPdfSnapshotExportDialog(HWND owner) {
    if (pending_pdf_export.paragraphs.empty()) return false;
    const int result = export_paragraphs_to_pdf_dialog(
        owner, pending_pdf_export.paragraphs, pending_pdf_export.settings);
    pending_pdf_export = {};
    return result;
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
