#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#define USEBCM
#include "opuscmd.h"

namespace {

constexpr wchar_t kToolbarClass[] = L"OpusWin95Toolbar";
constexpr int kToolbarBitmap = 201;
constexpr int kSpriteCell = 20;
constexpr COLORREF kButtonFace = RGB(192, 192, 192);
constexpr COLORREF kButtonShadow = RGB(128, 128, 128);
constexpr COLORREF kButtonDarkShadow = RGB(0, 0, 0);
constexpr COLORREF kButtonHighlight = RGB(255, 255, 255);
constexpr COLORREF kCaptionBlue = RGB(0, 0, 128);
constexpr UINT kComboStyle = 0x7501;
constexpr UINT kComboFont = 0x7502;
constexpr UINT kComboSize = 0x7503;
constexpr UINT_PTR kSyncTimer = 0x951;

enum class FormatGlyph {
    bold,
    italic,
    underline,
    color,
    align_left,
    align_center,
    align_right,
    align_justify,
    numbered,
    indent_left,
    indent_right,
    table
};

struct SpriteButton {
    int sprite;
    UINT command;
    int group;
    bool latch;
};

struct FormatButton {
    FormatGlyph glyph;
    UINT command;
    int group;
    bool latch;
};

constexpr std::array<SpriteButton, 17> kStandardButtons{{
    {0, bcmFileNew, 0, false},
    {1, imiOpen, 0, false},
    {2, bcmSave, 0, false},
    {3, bcmPrint, 1, false},
    {4, bcmPrintPreview, 1, false},
    {5, imiSpelling, 1, false},
    {6, bcmCut, 2, false},
    {7, bcmCopy, 2, false},
    {8, bcmPaste, 2, false},
    {9, bcmCopyLooks, 2, false},
    {10, bcmUndo, 3, false},
    {11, bcmRepeat, 3, false},
    {12, bcmInsTable, 4, false},
    {13, imiRenumParas, 4, false},
    {14, bcmShowAll, 4, true},
    {15, bcmHelp, 5, false},
    {16, bcmHelp, 5, false},
}};

constexpr std::array<FormatButton, 12> kFormatButtons{{
    {FormatGlyph::bold, bcmBold, 0, true},
    {FormatGlyph::italic, bcmItalic, 0, true},
    {FormatGlyph::underline, bcmULine, 0, true},
    {FormatGlyph::color, bcmColor, 0, false},
    {FormatGlyph::align_left, bcmParaLeft, 1, true},
    {FormatGlyph::align_center, bcmParaCenter, 1, true},
    {FormatGlyph::align_right, bcmParaRight, 1, true},
    {FormatGlyph::align_justify, bcmParaBoth, 1, true},
    {FormatGlyph::numbered, imiRenumParas, 2, false},
    {FormatGlyph::indent_left, bcmUnIndent, 2, false},
    {FormatGlyph::indent_right, bcmIndent, 2, false},
    {FormatGlyph::table, bcmInsTable, 3, false},
}};

struct HitResult {
    bool hit = false;
    bool format = false;
    int index = -1;
    UINT command = 0;
};

struct ToolbarState {
    HBITMAP sprite = nullptr;
    HFONT font = nullptr;
    HWND style_combo = nullptr;
    HWND font_combo = nullptr;
    HWND size_combo = nullptr;
    HWND source_style = nullptr;
    HWND source_font = nullptr;
    HWND source_size = nullptr;
    int copied_style_count = -1;
    int copied_font_count = -1;
    int copied_size_count = -1;
    ULONGLONG suppress_sync_until = 0;
    bool style_edit_dirty = false;
    bool font_edit_dirty = false;
    bool size_edit_dirty = false;
    HitResult pressed{};
    std::array<bool, kStandardButtons.size()> standard_latched{};
    std::array<bool, kFormatButtons.size()> format_latched{};
};

HBRUSH g_menu_brush = nullptr;
HMENU g_table_menu = nullptr;

int dpi_for_window(HWND window) {
    using GetDpiForWindowProc = UINT(WINAPI*)(HWND);
    static const auto get_dpi = reinterpret_cast<GetDpiForWindowProc>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    return get_dpi != nullptr && window != nullptr ?
               static_cast<int>(get_dpi(window)) :
               96;
}

int scale(HWND window, int value) {
    return MulDiv(value, dpi_for_window(window), 96);
}

void set_window_classic(HWND window) {
    using SetWindowThemeProc = HRESULT(WINAPI*)(HWND, LPCWSTR, LPCWSTR);
    static HMODULE theme_module = LoadLibraryW(L"uxtheme.dll");
    static const auto set_theme = theme_module != nullptr ?
        reinterpret_cast<SetWindowThemeProc>(
            GetProcAddress(theme_module, "SetWindowTheme")) : nullptr;
    if (set_theme != nullptr && window != nullptr) {
        set_theme(window, L"", L"");
    }
}

void style_menu_tree(HMENU menu) {
    if (menu == nullptr) {
        return;
    }
    MENUINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
    info.hbrBack = g_menu_brush;
    SetMenuInfo(menu, &info);
    const int count = GetMenuItemCount(menu);
    for (int index = 0; index < count; ++index) {
        style_menu_tree(GetSubMenu(menu, index));
    }
}

void configure_word95_menus(HWND window) {
    HMENU root = GetMenu(window);
    if (root == nullptr || GetMenuItemCount(root) < 7) {
        return;
    }

    auto menu_name = [root](int index) {
        wchar_t label[80]{};
        GetMenuStringW(root, index, label,
                       static_cast<int>(sizeof(label) / sizeof(label[0])),
                       MF_BYPOSITION);
        std::wstring name;
        for (const wchar_t character : std::wstring(label)) {
            if (character != L'&') {
                name.push_back(character);
            }
        }
        return name;
    };
    auto find_named = [&menu_name, root](const wchar_t* expected) {
        const int count = GetMenuItemCount(root);
        for (int index = 0; index < count; ++index) {
            if (lstrcmpiW(menu_name(index).c_str(), expected) == 0) {
                return index;
            }
        }
        return -1;
    };

    // Word's menu loader can finish replacing the startup menu after this
    // layer is created, so normalize by label every time we resync.
    const int file_index = find_named(L"File");
    if (file_index >= 0 && GetMenuItemCount(root) > file_index + 4) {
        HMENU insert = GetSubMenu(root, file_index + 3);
        HMENU format = GetSubMenu(root, file_index + 4);
        if (insert != nullptr) {
            ModifyMenuW(root, file_index + 3,
                        MF_BYPOSITION | MF_POPUP | MF_STRING,
                        reinterpret_cast<UINT_PTR>(insert), L"&Insert");
        }
        if (format != nullptr) {
            ModifyMenuW(root, file_index + 4,
                        MF_BYPOSITION | MF_POPUP | MF_STRING,
                        reinterpret_cast<UINT_PTR>(format), L"F&ormat");
        }
    }
    const int utilities_index = find_named(L"Utilities");
    if (utilities_index >= 0) {
        HMENU tools = GetSubMenu(root, utilities_index);
        ModifyMenuW(root, utilities_index,
                    MF_BYPOSITION | MF_POPUP | MF_STRING,
                    reinterpret_cast<UINT_PTR>(tools), L"&Tools");
    }

    if (g_table_menu == nullptr || !IsMenu(g_table_menu)) {
        g_table_menu = CreatePopupMenu();
        if (g_table_menu != nullptr) {
            AppendMenuW(g_table_menu, MF_STRING, bcmInsTable,
                        L"&Insert Table...");
            AppendMenuW(g_table_menu, MF_STRING, bcmTableToText,
                        L"Convert Table to Te&xt...");
            AppendMenuW(g_table_menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(g_table_menu, MF_STRING, imiFormatTable,
                        L"&Format Table...");
        }
    }
    for (int index = GetMenuItemCount(root) - 1; index >= 0; --index) {
        if (GetSubMenu(root, index) == g_table_menu) {
            RemoveMenu(root, index, MF_BYPOSITION);
        }
    }
    const int window_index = find_named(L"Window");
    if (g_table_menu != nullptr && window_index >= 0) {
        InsertMenuW(root, window_index,
                    MF_BYPOSITION | MF_POPUP | MF_STRING,
                    reinterpret_cast<UINT_PTR>(g_table_menu), L"Ta&ble");
    }
}

void apply_caption_colors(HWND window) {
    using DwmSetWindowAttributeProc = HRESULT(WINAPI*)(HWND, DWORD,
                                                       LPCVOID, DWORD);
    HMODULE module = LoadLibraryW(L"dwmapi.dll");
    if (module == nullptr) {
        return;
    }
    const auto set_attribute = reinterpret_cast<DwmSetWindowAttributeProc>(
        GetProcAddress(module, "DwmSetWindowAttribute"));
    if (set_attribute != nullptr) {
        constexpr DWORD kDwmBorderColor = 34;
        constexpr DWORD kDwmCaptionColor = 35;
        constexpr DWORD kDwmTextColor = 36;
        constexpr DWORD kDwmCornerPreference = 33;
        const COLORREF border = kButtonShadow;
        const COLORREF caption = kCaptionBlue;
        const COLORREF text = RGB(255, 255, 255);
        const DWORD do_not_round = 1;
        set_attribute(window, kDwmBorderColor, &border, sizeof(border));
        set_attribute(window, kDwmCaptionColor, &caption, sizeof(caption));
        set_attribute(window, kDwmTextColor, &text, sizeof(text));
        set_attribute(window, kDwmCornerPreference, &do_not_round,
                      sizeof(do_not_round));
    }
    FreeLibrary(module);
}

RECT standard_button_rect(HWND toolbar, int target) {
    const int button = scale(toolbar, 27);
    const int gap = scale(toolbar, 2);
    const int group_gap = scale(toolbar, 7);
    int x = scale(toolbar, 4);
    int previous_group = kStandardButtons[0].group;
    for (int index = 0; index <= target; ++index) {
        if (index > 0 && kStandardButtons[index].group != previous_group) {
            x += group_gap;
        }
        if (index == target) {
            return RECT{x, scale(toolbar, 2), x + button,
                        scale(toolbar, 2) + button};
        }
        x += button + gap;
        previous_group = kStandardButtons[index].group;
    }
    return RECT{};
}

int format_buttons_start(HWND toolbar) {
    return scale(toolbar, 4 + 118 + 4 + 146 + 4 + 52 + 8);
}

RECT format_button_rect(HWND toolbar, int target) {
    const int button = scale(toolbar, 27);
    const int gap = scale(toolbar, 2);
    const int group_gap = scale(toolbar, 7);
    int x = format_buttons_start(toolbar);
    int previous_group = kFormatButtons[0].group;
    for (int index = 0; index <= target; ++index) {
        if (index > 0 && kFormatButtons[index].group != previous_group) {
            x += group_gap;
        }
        if (index == target) {
            return RECT{x, scale(toolbar, 34), x + button,
                        scale(toolbar, 34) + button};
        }
        x += button + gap;
        previous_group = kFormatButtons[index].group;
    }
    return RECT{};
}

void draw_classic_edge(HDC dc, RECT rect, bool sunken) {
    const HPEN top_outer = CreatePen(PS_SOLID, 1,
        sunken ? kButtonDarkShadow : kButtonHighlight);
    const HPEN top_inner = CreatePen(PS_SOLID, 1,
        sunken ? kButtonShadow : kButtonFace);
    const HPEN bottom_outer = CreatePen(PS_SOLID, 1,
        sunken ? kButtonHighlight : kButtonDarkShadow);
    const HPEN bottom_inner = CreatePen(PS_SOLID, 1,
        sunken ? kButtonFace : kButtonShadow);
    HPEN old = static_cast<HPEN>(SelectObject(dc, top_outer));
    MoveToEx(dc, rect.left, rect.bottom - 1, nullptr);
    LineTo(dc, rect.left, rect.top);
    LineTo(dc, rect.right - 1, rect.top);
    SelectObject(dc, top_inner);
    MoveToEx(dc, rect.left + 1, rect.bottom - 2, nullptr);
    LineTo(dc, rect.left + 1, rect.top + 1);
    LineTo(dc, rect.right - 2, rect.top + 1);
    SelectObject(dc, bottom_outer);
    MoveToEx(dc, rect.left, rect.bottom - 1, nullptr);
    LineTo(dc, rect.right - 1, rect.bottom - 1);
    LineTo(dc, rect.right - 1, rect.top - 1);
    SelectObject(dc, bottom_inner);
    MoveToEx(dc, rect.left + 1, rect.bottom - 2, nullptr);
    LineTo(dc, rect.right - 2, rect.bottom - 2);
    LineTo(dc, rect.right - 2, rect.top);
    SelectObject(dc, old);
    DeleteObject(top_outer);
    DeleteObject(top_inner);
    DeleteObject(bottom_outer);
    DeleteObject(bottom_inner);
}

void draw_standard_glyph(HWND toolbar, HDC dc, HDC sprite_dc,
                         const SpriteButton& spec, RECT rect, bool sunken) {
    const int glyph = scale(toolbar, 20);
    const int offset = sunken ? scale(toolbar, 1) : 0;
    const int x = rect.left + (rect.right - rect.left - glyph) / 2 + offset;
    const int y = rect.top + (rect.bottom - rect.top - glyph) / 2 + offset;
    SetStretchBltMode(dc, COLORONCOLOR);
    StretchBlt(dc, x, y, glyph, glyph, sprite_dc,
               spec.sprite * kSpriteCell, 0, kSpriteCell, kSpriteCell,
               SRCCOPY);
}

void draw_alignment(HDC dc, RECT area, FormatGlyph glyph) {
    const int width = area.right - area.left;
    const int left = area.left + 2;
    const int right = area.right - 2;
    for (int row = 0; row < 4; ++row) {
        const int y = area.top + 2 + row * 4;
        int x1 = left;
        int x2 = right;
        const int short_by = (row % 2 == 0) ? width / 4 : width / 6;
        if (glyph == FormatGlyph::align_left) {
            x2 -= short_by;
        } else if (glyph == FormatGlyph::align_center) {
            x1 += short_by / 2;
            x2 -= short_by / 2;
        } else if (glyph == FormatGlyph::align_right) {
            x1 += short_by;
        } else if (glyph == FormatGlyph::align_justify && row == 3) {
            x2 -= width / 5;
        }
        MoveToEx(dc, x1, y, nullptr);
        LineTo(dc, x2, y);
    }
}

void draw_format_glyph(HWND toolbar, HDC dc, const FormatButton& spec,
                       RECT rect, bool sunken, HDC sprite_dc) {
    const int offset = sunken ? scale(toolbar, 1) : 0;
    RECT area{rect.left + scale(toolbar, 4) + offset,
              rect.top + scale(toolbar, 4) + offset,
              rect.right - scale(toolbar, 4) + offset,
              rect.bottom - scale(toolbar, 4) + offset};
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(0, 0, 0));
    HFONT old_font = nullptr;
    LOGFONTW logical{};
    logical.lfHeight = -(area.bottom - area.top);
    lstrcpyW(logical.lfFaceName, L"Arial");
    if (spec.glyph == FormatGlyph::bold) {
        logical.lfWeight = FW_BOLD;
    } else if (spec.glyph == FormatGlyph::italic) {
        logical.lfItalic = TRUE;
    } else if (spec.glyph == FormatGlyph::underline) {
        logical.lfUnderline = TRUE;
    }
    if (spec.glyph == FormatGlyph::bold ||
        spec.glyph == FormatGlyph::italic ||
        spec.glyph == FormatGlyph::underline ||
        spec.glyph == FormatGlyph::color) {
        HFONT font = CreateFontIndirectW(&logical);
        old_font = static_cast<HFONT>(SelectObject(dc, font));
        DrawTextW(dc, spec.glyph == FormatGlyph::color ? L"A" :
                  spec.glyph == FormatGlyph::bold ? L"B" :
                  spec.glyph == FormatGlyph::italic ? L"I" : L"U",
                  1, &area, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(dc, old_font);
        DeleteObject(font);
        if (spec.glyph == FormatGlyph::color) {
            HBRUSH swatch = CreateSolidBrush(RGB(255, 255, 0));
            RECT swatch_rect{area.left + 1, area.bottom - 3,
                             area.right - 1, area.bottom};
            FillRect(dc, &swatch_rect, swatch);
            DeleteObject(swatch);
        }
        return;
    }
    if (spec.glyph == FormatGlyph::align_left ||
        spec.glyph == FormatGlyph::align_center ||
        spec.glyph == FormatGlyph::align_right ||
        spec.glyph == FormatGlyph::align_justify) {
        draw_alignment(dc, area, spec.glyph);
        return;
    }
    if (spec.glyph == FormatGlyph::numbered) {
        for (int row = 0; row < 3; ++row) {
            wchar_t number[2]{static_cast<wchar_t>(L'1' + row), L'\0'};
            TextOutW(dc, area.left, area.top + row * 5 - 1, number, 1);
            MoveToEx(dc, area.left + 6, area.top + row * 5 + 2, nullptr);
            LineTo(dc, area.right, area.top + row * 5 + 2);
        }
        return;
    }
    if (spec.glyph == FormatGlyph::indent_left ||
        spec.glyph == FormatGlyph::indent_right) {
        for (int row = 0; row < 3; ++row) {
            MoveToEx(dc, area.left + 6, area.top + 2 + row * 5, nullptr);
            LineTo(dc, area.right, area.top + 2 + row * 5);
        }
        const bool right = spec.glyph == FormatGlyph::indent_right;
        POINT triangle[3]{{right ? area.left + 1 : area.left + 6, area.top + 6},
                          {right ? area.left + 6 : area.left + 1, area.top + 3},
                          {right ? area.left + 6 : area.left + 1, area.top + 9}};
        HBRUSH brush = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        HBRUSH old_brush = static_cast<HBRUSH>(SelectObject(dc, brush));
        Polygon(dc, triangle, 3);
        SelectObject(dc, old_brush);
        return;
    }
    if (spec.glyph == FormatGlyph::table && sprite_dc != nullptr) {
        const int glyph = area.bottom - area.top;
        StretchBlt(dc, area.left, area.top, glyph, glyph, sprite_dc,
                   12 * kSpriteCell, 0, kSpriteCell, kSpriteCell, SRCCOPY);
    }
}

void paint_toolbar(HWND toolbar, ToolbarState& state) {
    PAINTSTRUCT paint{};
    HDC dc = BeginPaint(toolbar, &paint);
    RECT client{};
    GetClientRect(toolbar, &client);
    HBRUSH face = CreateSolidBrush(kButtonFace);
    FillRect(dc, &client, face);
    DeleteObject(face);

    HPEN separator = CreatePen(PS_SOLID, 1, kButtonShadow);
    HPEN highlight = CreatePen(PS_SOLID, 1, kButtonHighlight);
    HPEN old_pen = static_cast<HPEN>(SelectObject(dc, separator));
    const int row_line = scale(toolbar, 32);
    MoveToEx(dc, 0, row_line, nullptr);
    LineTo(dc, client.right, row_line);
    SelectObject(dc, highlight);
    MoveToEx(dc, 0, row_line + 1, nullptr);
    LineTo(dc, client.right, row_line + 1);
    SelectObject(dc, old_pen);
    DeleteObject(separator);
    DeleteObject(highlight);

    HDC sprite_dc = CreateCompatibleDC(dc);
    HBITMAP old_bitmap = state.sprite != nullptr ?
        static_cast<HBITMAP>(SelectObject(sprite_dc, state.sprite)) : nullptr;
    for (int index = 0; index < static_cast<int>(kStandardButtons.size());
         ++index) {
        RECT rect = standard_button_rect(toolbar, index);
        const bool down =
            (state.pressed.hit && !state.pressed.format &&
             state.pressed.index == index) || state.standard_latched[index];
        draw_classic_edge(dc, rect, down);
        if (state.sprite != nullptr) {
            draw_standard_glyph(toolbar, dc, sprite_dc,
                                kStandardButtons[index], rect, down);
        }
    }
    for (int index = 0; index < static_cast<int>(kFormatButtons.size());
         ++index) {
        RECT rect = format_button_rect(toolbar, index);
        const bool down =
            (state.pressed.hit && state.pressed.format &&
             state.pressed.index == index) || state.format_latched[index];
        draw_classic_edge(dc, rect, down);
        draw_format_glyph(toolbar, dc, kFormatButtons[index], rect, down,
                          state.sprite != nullptr ? sprite_dc : nullptr);
    }
    if (old_bitmap != nullptr) {
        SelectObject(sprite_dc, old_bitmap);
    }
    DeleteDC(sprite_dc);
    EndPaint(toolbar, &paint);
}

HitResult hit_test(HWND toolbar, POINT point) {
    for (int index = 0; index < static_cast<int>(kStandardButtons.size());
         ++index) {
        RECT rect = standard_button_rect(toolbar, index);
        if (PtInRect(&rect, point)) {
            return {true, false, index, kStandardButtons[index].command};
        }
    }
    for (int index = 0; index < static_cast<int>(kFormatButtons.size());
         ++index) {
        RECT rect = format_button_rect(toolbar, index);
        if (PtInRect(&rect, point)) {
            return {true, true, index, kFormatButtons[index].command};
        }
    }
    return {};
}

bool is_toolbar_descendant(HWND toolbar, HWND candidate) {
    return candidate == toolbar || IsChild(toolbar, candidate) != FALSE;
}

std::string combo_item(HWND combo, int index) {
    const LRESULT length = SendMessageA(combo, CB_GETLBTEXTLEN, index, 0);
    if (length == CB_ERR || length < 0) {
        return {};
    }
    std::vector<char> text(static_cast<std::size_t>(length) + 1);
    SendMessageA(combo, CB_GETLBTEXT, index,
                 reinterpret_cast<LPARAM>(text.data()));
    return text.data();
}

bool combo_contains(HWND combo, const char* value) {
    return SendMessageA(combo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1),
                        reinterpret_cast<LPARAM>(value)) != CB_ERR;
}

struct ComboEnumeration {
    HWND toolbar;
    std::vector<HWND> combos;
};

BOOL CALLBACK collect_original_combos(HWND candidate, LPARAM parameter) {
    auto& enumeration = *reinterpret_cast<ComboEnumeration*>(parameter);
    if (is_toolbar_descendant(enumeration.toolbar, candidate)) {
        return TRUE;
    }
    wchar_t class_name[64]{};
    GetClassNameW(candidate, class_name,
                  static_cast<int>(sizeof(class_name) / sizeof(class_name[0])));
    if (lstrcmpiW(class_name, L"ComboBox") == 0) {
        enumeration.combos.push_back(candidate);
    }
    return TRUE;
}

void locate_source_combos(HWND toolbar, ToolbarState& state) {
    ComboEnumeration enumeration{toolbar, {}};
    EnumChildWindows(GetParent(toolbar), collect_original_combos,
                     reinterpret_cast<LPARAM>(&enumeration));
    for (HWND combo : enumeration.combos) {
        if (combo_contains(combo, "Courier New") &&
            combo_contains(combo, "Arial")) {
            state.source_font = combo;
        } else if (combo_contains(combo, "24") &&
                   combo_contains(combo, "72")) {
            state.source_size = combo;
        } else if (combo_contains(combo, "Normal") ||
                   SendMessageA(combo, CB_GETCOUNT, 0, 0) > 0) {
            state.source_style = combo;
        }
    }
}

std::wstring wide_from_ansi(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int count = MultiByteToWideChar(CP_ACP, 0, text.c_str(), -1,
                                          nullptr, 0);
    std::vector<wchar_t> wide(static_cast<std::size_t>(count));
    MultiByteToWideChar(CP_ACP, 0, text.c_str(), -1, wide.data(), count);
    return wide.data();
}

std::string ansi_from_wide(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int count = WideCharToMultiByte(CP_ACP, 0, text.c_str(), -1,
                                          nullptr, 0, nullptr, nullptr);
    std::vector<char> ansi(static_cast<std::size_t>(count));
    WideCharToMultiByte(CP_ACP, 0, text.c_str(), -1, ansi.data(), count,
                        nullptr, nullptr);
    return ansi.data();
}

bool combo_or_child_has_focus(HWND combo) {
    const HWND focus = GetFocus();
    return focus == combo || (focus != nullptr && IsChild(combo, focus));
}

void sync_combo(HWND mirror, HWND source, int& copied_count) {
    if (mirror == nullptr || source == nullptr || !IsWindow(source)) {
        return;
    }
    const int count = static_cast<int>(SendMessageA(source, CB_GETCOUNT, 0, 0));
    if (count >= 0 && count != copied_count) {
        std::wstring mirror_text;
        const int mirror_length = GetWindowTextLengthW(mirror);
        mirror_text.resize(static_cast<std::size_t>(mirror_length) + 1);
        GetWindowTextW(mirror, &mirror_text[0], mirror_length + 1);
        SendMessageW(mirror, CB_RESETCONTENT, 0, 0);
        for (int index = 0; index < count; ++index) {
            const std::wstring item = wide_from_ansi(combo_item(source, index));
            SendMessageW(mirror, CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(item.c_str()));
        }
        SetWindowTextW(mirror, mirror_text.c_str());
        copied_count = count;
    }
    if (!combo_or_child_has_focus(mirror)) {
        const LRESULT selection = SendMessageA(source, CB_GETCURSEL, 0, 0);
        if (selection != CB_ERR && selection < count) {
            SendMessageW(mirror, CB_SETCURSEL, selection, 0);
        } else {
            const int length = GetWindowTextLengthA(source);
            std::vector<char> text(static_cast<std::size_t>(length) + 1);
            GetWindowTextA(source, text.data(), static_cast<int>(text.size()));
            const std::wstring source_text = wide_from_ansi(text.data());
            if (!source_text.empty()) {
                SetWindowTextW(mirror, source_text.c_str());
            }
        }
        SendMessageW(mirror, CB_SETEDITSEL, 0, MAKELPARAM(-1, 0));
    }
}

void sync_mirrors(HWND toolbar, ToolbarState& state) {
    if (state.source_font == nullptr || !IsWindow(state.source_font) ||
        state.source_size == nullptr || !IsWindow(state.source_size) ||
        state.source_style == nullptr || !IsWindow(state.source_style)) {
        locate_source_combos(toolbar, state);
    }
    sync_combo(state.style_combo, state.source_style,
               state.copied_style_count);
    sync_combo(state.font_combo, state.source_font, state.copied_font_count);
    sync_combo(state.size_combo, state.source_size, state.copied_size_count);
}

BOOL CALLBACK find_document_pane(HWND candidate, LPARAM parameter) {
    wchar_t class_name[64]{};
    GetClassNameW(candidate, class_name,
                  static_cast<int>(sizeof(class_name) / sizeof(class_name[0])));
    if (lstrcmpiW(class_name, L"OpusWwd") == 0) {
        *reinterpret_cast<HWND*>(parameter) = candidate;
        return FALSE;
    }
    return TRUE;
}

void restore_document_focus(HWND source) {
    HWND pane = nullptr;
    EnumChildWindows(GetAncestor(source, GA_ROOT), find_document_pane,
                     reinterpret_cast<LPARAM>(&pane));
    if (pane != nullptr) {
        SetFocus(pane);
    }
}

void forward_combo(HWND mirror, HWND source, int notification,
                   bool& edit_dirty) {
    if (mirror == nullptr || source == nullptr || !IsWindow(source)) {
        return;
    }
    if (notification == CBN_SELCHANGE || notification == CBN_SELENDOK) {
        edit_dirty = false;
    }
    std::wstring wide;
    const LRESULT mirror_selection = SendMessageW(mirror, CB_GETCURSEL, 0, 0);
    if (mirror_selection != CB_ERR) {
        const LRESULT length = SendMessageW(
            mirror, CB_GETLBTEXTLEN, mirror_selection, 0);
        wide.resize(static_cast<std::size_t>(length) + 1, L'\0');
        SendMessageW(mirror, CB_GETLBTEXT, mirror_selection,
                     reinterpret_cast<LPARAM>(&wide[0]));
    } else {
        const int length = GetWindowTextLengthW(mirror);
        wide.resize(static_cast<std::size_t>(length) + 1, L'\0');
        GetWindowTextW(mirror, &wide[0], length + 1);
    }
    const std::string text = ansi_from_wide(wide.c_str());
    const LRESULT selection = SendMessageA(
        source, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1),
        reinterpret_cast<LPARAM>(text.c_str()));
    if (selection != CB_ERR) {
        SendMessageA(source, CB_SETCURSEL, selection, 0);
    }
    SetWindowTextA(source, text.c_str());
    const HWND parent = GetParent(source);
    const int control_id = GetDlgCtrlID(source);
    if (notification == CBN_SELCHANGE) {
        SendMessageW(parent, WM_COMMAND,
                     MAKEWPARAM(control_id, CBN_SELCHANGE),
                     reinterpret_cast<LPARAM>(source));
    } else if (notification == CBN_SELENDOK) {
        SendMessageW(parent, WM_COMMAND,
                     MAKEWPARAM(control_id, CBN_SELENDOK),
                     reinterpret_cast<LPARAM>(source));
        restore_document_focus(source);
    } else if (notification == CBN_EDITCHANGE) {
        // A dropdown selection can emit EDITCHANGE too; only free-form text
        // (no selected list item) needs the legacy control's focus-loss
        // commit path.
        edit_dirty = SendMessageW(mirror, CB_GETCURSEL, 0, 0) == CB_ERR;
        SendMessageW(parent, WM_COMMAND,
                     MAKEWPARAM(control_id, CBN_EDITCHANGE),
                     reinterpret_cast<LPARAM>(source));
    } else if (notification == CBN_DROPDOWN) {
        SendMessageW(parent, WM_COMMAND,
                     MAKEWPARAM(control_id, CBN_DROPDOWN),
                     reinterpret_cast<LPARAM>(source));
    } else if (notification == CBN_KILLFOCUS && edit_dirty) {
        SendMessageW(parent, WM_COMMAND,
                     MAKEWPARAM(control_id, CBN_KILLFOCUS),
                     reinterpret_cast<LPARAM>(source));
        edit_dirty = false;
    }
}

void position_combos(HWND toolbar, ToolbarState& state) {
    const int y = scale(toolbar, 35);
    // For CBS_DROPDOWN the window height also reserves the popup list. The
    // visible, closed control still uses its system edit-field height.
    const int height = scale(toolbar, 220);
    const int x_style = scale(toolbar, 4);
    const int width_style = scale(toolbar, 118);
    const int x_font = scale(toolbar, 126);
    const int width_font = scale(toolbar, 146);
    const int x_size = scale(toolbar, 276);
    const int width_size = scale(toolbar, 52);
    MoveWindow(state.style_combo, x_style, y, width_style, height, TRUE);
    MoveWindow(state.font_combo, x_font, y, width_font, height, TRUE);
    MoveWindow(state.size_combo, x_size, y, width_size, height, TRUE);
}

HWND create_combo(HWND toolbar, UINT id) {
    HWND combo = CreateWindowExW(
        0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
            CBS_DROPDOWN | CBS_AUTOHSCROLL,
        0, 0, 10, 200, toolbar,
        reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),
        GetModuleHandleW(nullptr), nullptr);
    set_window_classic(combo);
    return combo;
}

LRESULT CALLBACK toolbar_window_proc(HWND window, UINT message,
                                     WPARAM w_param, LPARAM l_param) {
    auto* state = reinterpret_cast<ToolbarState*>(
        GetWindowLongPtrW(window, GWLP_USERDATA));
    switch (message) {
    case WM_CREATE: {
        auto* created = new ToolbarState{};
        SetWindowLongPtrW(window, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(created));
        created->sprite = static_cast<HBITMAP>(LoadImageW(
            GetModuleHandleW(nullptr), MAKEINTRESOURCEW(kToolbarBitmap),
            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
        LOGFONTW logical{};
        logical.lfHeight = -MulDiv(8, dpi_for_window(window), 72);
        logical.lfWeight = FW_NORMAL;
        logical.lfCharSet = DEFAULT_CHARSET;
        lstrcpyW(logical.lfFaceName, L"MS Sans Serif");
        created->font = CreateFontIndirectW(&logical);
        created->style_combo = create_combo(window, kComboStyle);
        created->font_combo = create_combo(window, kComboFont);
        created->size_combo = create_combo(window, kComboSize);
        SetWindowTextW(created->style_combo, L"Normal");
        SetWindowTextW(created->font_combo, L"Arial");
        SetWindowTextW(created->size_combo, L"10");
        SendMessageW(created->style_combo, WM_SETFONT,
                     reinterpret_cast<WPARAM>(created->font), FALSE);
        SendMessageW(created->font_combo, WM_SETFONT,
                     reinterpret_cast<WPARAM>(created->font), FALSE);
        SendMessageW(created->size_combo, WM_SETFONT,
                     reinterpret_cast<WPARAM>(created->font), FALSE);
        position_combos(window, *created);
        sync_mirrors(window, *created);
        SetTimer(window, kSyncTimer, 350, nullptr);
        return 0;
    }
    case WM_SIZE:
        if (state != nullptr) {
            position_combos(window, *state);
        }
        return 0;
    case WM_TIMER:
        if (state != nullptr && w_param == kSyncTimer) {
            if (GetTickCount64() >= state->suppress_sync_until) {
                sync_mirrors(window, *state);
            }
        }
        return 0;
    case WM_COMMAND:
        if (state != nullptr) {
            const UINT id = LOWORD(w_param);
            const int notification = HIWORD(w_param);
            if (notification == CBN_SELCHANGE ||
                notification == CBN_SELENDOK ||
                notification == CBN_EDITCHANGE) {
                state->suppress_sync_until = GetTickCount64() + 1000;
            }
            if (id == kComboStyle) {
                forward_combo(state->style_combo, state->source_style,
                              notification, state->style_edit_dirty);
                return 0;
            }
            if (id == kComboFont) {
                forward_combo(state->font_combo, state->source_font,
                              notification, state->font_edit_dirty);
                return 0;
            }
            if (id == kComboSize) {
                forward_combo(state->size_combo, state->source_size,
                              notification, state->size_edit_dirty);
                return 0;
            }
        }
        break;
    case WM_LBUTTONDOWN:
        if (state != nullptr) {
            POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
            state->pressed = hit_test(window, point);
            if (state->pressed.hit) {
                SetCapture(window);
                InvalidateRect(window, nullptr, FALSE);
            }
        }
        return 0;
    case WM_LBUTTONUP:
        if (state != nullptr && state->pressed.hit) {
            POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
            const HitResult released = hit_test(window, point);
            const HitResult pressed = state->pressed;
            state->pressed = {};
            if (GetCapture() == window) {
                ReleaseCapture();
            }
            if (released.hit && released.format == pressed.format &&
                released.index == pressed.index) {
                if (pressed.format &&
                    kFormatButtons[pressed.index].latch) {
                    if (pressed.index >= 4 && pressed.index <= 7) {
                        for (int index = 4; index <= 7; ++index) {
                            state->format_latched[index] = false;
                        }
                    }
                    state->format_latched[pressed.index] =
                        !state->format_latched[pressed.index];
                } else if (!pressed.format &&
                           kStandardButtons[pressed.index].latch) {
                    state->standard_latched[pressed.index] =
                        !state->standard_latched[pressed.index];
                }
                PostMessageW(GetParent(window), WM_COMMAND,
                             MAKEWPARAM(pressed.command, 0), 0);
            }
            InvalidateRect(window, nullptr, FALSE);
        }
        return 0;
    case WM_CAPTURECHANGED:
        if (state != nullptr && state->pressed.hit) {
            state->pressed = {};
            InvalidateRect(window, nullptr, FALSE);
        }
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        if (state != nullptr) {
            paint_toolbar(window, *state);
            return 0;
        }
        break;
    case WM_DESTROY:
        if (state != nullptr) {
            KillTimer(window, kSyncTimer);
            if (state->sprite != nullptr) {
                DeleteObject(state->sprite);
            }
            if (state->font != nullptr) {
                DeleteObject(state->font);
            }
            delete state;
            SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        }
        break;
    }
    return DefWindowProcW(window, message, w_param, l_param);
}

bool register_toolbar_class() {
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_DBLCLKS;
    window_class.lpfnWndProc = toolbar_window_proc;
    window_class.hInstance = GetModuleHandleW(nullptr);
    window_class.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    window_class.lpszClassName = kToolbarClass;
    return RegisterClassExW(&window_class) != 0 ||
           GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

}  // namespace

extern "C" HWND vhwndWin95Toolbar = nullptr;

extern "C" int OpusWin95ToolbarHeight(void) {
    return scale(vhwndWin95Toolbar, 64);
}

extern "C" int OpusWin95ChromeActive(void) {
    return vhwndWin95Toolbar != nullptr && IsWindow(vhwndWin95Toolbar);
}

extern "C" void OpusSyncWin95Toolbar(void) {
    if (OpusWin95ChromeActive()) {
        SendMessageW(vhwndWin95Toolbar, WM_TIMER, kSyncTimer, 0);
        configure_word95_menus(GetParent(vhwndWin95Toolbar));
        style_menu_tree(GetMenu(GetParent(vhwndWin95Toolbar)));
        DrawMenuBar(GetParent(vhwndWin95Toolbar));
    }
}

extern "C" void OpusSizeWin95Toolbar(HWND parent) {
    if (!OpusWin95ChromeActive()) {
        return;
    }
    RECT client{};
    GetClientRect(parent, &client);
    MoveWindow(vhwndWin95Toolbar, 0, 0, client.right - client.left,
               OpusWin95ToolbarHeight(), TRUE);
}

extern "C" int OpusCreateWin95Chrome(HWND parent) {
    if (OpusWin95ChromeActive()) {
        return TRUE;
    }
    if (!register_toolbar_class()) {
        return FALSE;
    }
    if (g_menu_brush == nullptr) {
        g_menu_brush = CreateSolidBrush(kButtonFace);
    }
    configure_word95_menus(parent);
    style_menu_tree(GetMenu(parent));
    DrawMenuBar(parent);
    apply_caption_colors(parent);
    set_window_classic(parent);

    vhwndWin95Toolbar = CreateWindowExW(
        0, kToolbarClass, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        0, 0, 1, scale(parent, 64), parent, nullptr,
        GetModuleHandleW(nullptr), nullptr);
    if (vhwndWin95Toolbar == nullptr) {
        return FALSE;
    }
    OpusSizeWin95Toolbar(parent);
    SetWindowPos(vhwndWin95Toolbar, HWND_TOP, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    return TRUE;
}
