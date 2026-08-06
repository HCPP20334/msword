#include "opus_x64_compat.h"
extern "C" {
#include "dac.h"
}

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

extern "C" {
extern void** hcabDlgCur;
extern std::uint16_t wRefDlgCur;
}

/*
 * Flat, stateful implementation of the public SDM 2.21 dialog API used by
 * Opus.  The archive contains the SDM headers and 16-bit .obj files, but not
 * its C sources.  This layer retains the original CAB/TMC/HDLG contracts and
 * supplies the dialog state that the original Word modules expect.  Native
 * window creation is deliberately separate from this state layer.
 */

namespace {

using Word = std::uint16_t;
using Dword = std::uint32_t;
using Hdlg = Word;
using Tmc = Word;
using Hcab = void**;

struct Rec {
    int x;
    int y;
    int dx;
    int dy;
};

struct DltHeader {
    Rec rec;
    Word hid;
    Tmc tmc_sel_init;
    void* dialog_proc;
    Word base_item_count;
    Word border;
};

struct Dli {
    HWND hwnd;
    int dx;
    int dy;
    Dword flags;
    Word reference;
    unsigned char* runtime_items;
};

struct ControlState {
    Word value = 0;
    Dword selection = 0;
    bool enabled = true;
    bool visible = true;
    Word text_limit = 0xffff;
    Rec rectangle{};
    HWND window = nullptr;
    std::string text;
    std::vector<unsigned char> large_value;
    std::vector<std::string> entries;
};

struct DialogState {
    Hdlg handle = 0;
    DltHeader** template_handle = nullptr;
    Hcab cab = nullptr;
    HWND window = nullptr;
    Word reference = 0;
    Word hid = 0;
    Word sab = 0;
    Tmc focus = 0;
    Tmc default_tmc = 1;
    bool visible = false;
    bool dying = false;
    bool modal = false;
    std::string caption;
    std::unordered_map<Tmc, ControlState> controls;
};

std::unordered_map<Hdlg, DialogState> g_dialogs;
DialogState g_no_dialog;
Hdlg g_next_dialog = 1;
Hdlg g_current_dialog = 0;
Hdlg g_focus_dialog = 0;
bool g_initialized = false;
bool g_noninteractive = false;

void set_sds_handle(Hdlg current, Hdlg focus);

DialogState* find_dialog(const Hdlg dialog) {
    const auto found = g_dialogs.find(dialog);
    return found == g_dialogs.end() ? nullptr : &found->second;
}

DialogState& active_dialog() {
    auto* dialog = find_dialog(g_current_dialog);
    return dialog == nullptr ? g_no_dialog : *dialog;
}

#define g_dialog active_dialog()

ControlState& control(const Tmc tmc) {
    return g_dialog.controls[static_cast<Tmc>(tmc & ~0x8000u)];
}

const ControlState* find_control(const Tmc tmc) {
    const auto found =
        g_dialog.controls.find(static_cast<Tmc>(tmc & ~0x8000u));
    return found == g_dialog.controls.end() ? nullptr : &found->second;
}

void sync_current_dialog_globals() {
    const auto* dialog = find_dialog(g_current_dialog);
    hcabDlgCur = dialog == nullptr ? nullptr : dialog->cab;
    wRefDlgCur = dialog == nullptr ? 0 : dialog->reference;
    set_sds_handle(g_current_dialog, g_focus_dialog);
}

int scaled_x(const int value) {
    return MulDiv(value, dac.dxSysFontChar == 0 ? 8 : dac.dxSysFontChar, 4);
}

int scaled_y(const int value) {
    return MulDiv(value, dac.dySysFontChar == 0 ? 16 : dac.dySysFontChar, 8);
}

HWND create_dialog_host(const DialogState& dialog, const Dli* initializer) {
    if (initializer == nullptr || initializer->hwnd == nullptr ||
        !IsWindow(initializer->hwnd)) {
        return nullptr;
    }

    int x = initializer->dx;
    int y = initializer->dy;
    int width = 1;
    int height = 1;
    if (dialog.template_handle != nullptr &&
        *dialog.template_handle != nullptr) {
        const auto& rec = (*dialog.template_handle)->rec;
        x += scaled_x(rec.x);
        y += scaled_y(rec.y);
        width = (std::max)(1, scaled_x(rec.dx));
        height = (std::max)(1, scaled_y(rec.dy));
    }

    /* HdlgStartDlg is used by Opus only for ribbon/ruler child dialogs.
       Win16 propagated WM_SETVISIBLE when their icon-bar parent was shown;
       Win64 does not.  Keep the child style visible so parent visibility is
       inherited without depending on that retired message. */
    const DWORD style =
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    return CreateWindowExA(0, "STATIC", "", style, x, y, width, height,
                           initializer->hwnd, nullptr,
                           GetModuleHandleW(nullptr), nullptr);
}

HWND create_native_control(DialogState& dialog, const Tmc tmc,
                           const char* window_class, const char* caption,
                           const Rec& rectangle, const DWORD control_style) {
    if (dialog.window == nullptr || !IsWindow(dialog.window)) {
        return nullptr;
    }
    auto& state = dialog.controls[tmc];
    state.rectangle = rectangle;
    state.text = caption == nullptr ? "" : caption;
    state.window = CreateWindowExA(
        0, window_class, state.text.c_str(),
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | control_style,
        scaled_x(rectangle.x), scaled_y(rectangle.y),
        (std::max)(1, scaled_x(rectangle.dx)),
        (std::max)(1, scaled_y(rectangle.dy)), dialog.window,
        reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(tmc)),
        GetModuleHandleW(nullptr), nullptr);
    if (state.window != nullptr) {
        SendMessageA(state.window, WM_SETFONT,
                     reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),
                     false);
    }
    return state.window;
}

void create_static_text(DialogState& dialog, const char* caption,
                        const Rec& rectangle) {
    if (dialog.window == nullptr || !IsWindow(dialog.window)) {
        return;
    }
    const HWND window = CreateWindowExA(
        0, "STATIC", caption,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_LEFT,
        scaled_x(rectangle.x), scaled_y(rectangle.y),
        (std::max)(1, scaled_x(rectangle.dx)),
        (std::max)(1, scaled_y(rectangle.dy)), dialog.window, nullptr,
        GetModuleHandleW(nullptr), nullptr);
    if (window != nullptr) {
        SendMessageA(window, WM_SETFONT,
                     reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),
                     false);
    }
}

void materialize_icon_bar_template(DialogState& dialog) {
    constexpr Word cxtRibbonIconBar = 0x8005;
    constexpr Word cxtRulerIconBar = 0x8006;
    constexpr Tmc tmcUserMin = 0x0400;
    constexpr DWORD combo_style =
        WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL;

    if (dialog.hid == cxtRibbonIconBar) {
        const bool win3 = (*dialog.template_handle)->rec.dx == 160;
        if (win3) {
            create_static_text(dialog, "Font:", {4, 3, 20, 8});
            create_native_control(dialog, tmcUserMin, "COMBOBOX", "",
                                  {26, 1, 76, 67}, combo_style);
            create_static_text(dialog, "Pts:", {108, 3, 14, 8});
            create_native_control(dialog, tmcUserMin + 1, "COMBOBOX", "",
                                  {127, 1, 28, 67}, combo_style);
        } else {
            create_static_text(dialog, "Font:", {4, 3, 20, 8});
            create_native_control(dialog, tmcUserMin, "COMBOBOX", "",
                                  {29, 1, 80, 68}, combo_style);
            create_static_text(dialog, "Pts:", {115, 3, 16, 8});
            create_native_control(dialog, tmcUserMin + 1, "COMBOBOX", "",
                                  {134, 1, 32, 68}, combo_style);
        }
    } else if (dialog.hid == cxtRulerIconBar) {
        const bool win3 = (*dialog.template_handle)->rec.dx == 102;
        if (win3) {
            create_static_text(dialog, "Style:", {4, 3, 20, 8});
            create_native_control(dialog, tmcUserMin, "COMBOBOX", "",
                                  {26, 1, 76, 68}, combo_style);
        } else {
            create_static_text(dialog, "Style:", {4, 3, 24, 8});
            create_native_control(dialog, tmcUserMin, "COMBOBOX", "",
                                  {29, 1, 80, 68}, combo_style);
        }
    }
}

HWND ensure_control_window(const Tmc raw_tmc) {
    auto& state = control(raw_tmc);
    if (state.window != nullptr && IsWindow(state.window)) {
        return state.window;
    }
    if (g_dialog.window == nullptr || !IsWindow(g_dialog.window)) {
        return nullptr;
    }

    const Tmc tmc = static_cast<Tmc>(raw_tmc & ~0x8000u);
    DWORD style = WS_CHILD | WS_CLIPSIBLINGS;
    if (state.visible) {
        style |= WS_VISIBLE;
    }
    state.window = CreateWindowExA(
        0, "STATIC", state.text.c_str(), style, state.rectangle.x,
        state.rectangle.y, (std::max)(1, state.rectangle.dx),
        (std::max)(1, state.rectangle.dy), g_dialog.window,
        reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(tmc)),
        GetModuleHandleW(nullptr), nullptr);
    if (state.window != nullptr) {
        EnableWindow(state.window, state.enabled);
    }
    return state.window;
}

void copy_text(const std::string& source, char* destination,
               const Word capacity) {
    if (destination == nullptr || capacity == 0) {
        return;
    }
    const std::size_t count =
        (std::min)(source.size(), static_cast<std::size_t>(capacity - 1));
    std::memcpy(destination, source.data(), count);
    destination[count] = '\0';
}

std::string counted_or_zero_terminated(const char* text) {
    if (text == nullptr) {
        return {};
    }
    return std::string(text);
}

}  // namespace

extern "C" {

struct OpusSdsCompat {
    std::uintptr_t current_segment;
    std::uintptr_t focus_segment;
    void** current_dialog;
    void** focus_dialog;
};

OpusSdsCompat sds = {};
DAC_SDM dac = {};
HWND vhWndMsgBoxParent = nullptr;

extern void** hcabDlgCur;
extern Word wRefDlgCur;

int FInitSdm_sdm21(void*) {
    g_initialized = true;
    dac.dxBorder = GetSystemMetrics(SM_CXBORDER);
    dac.dyBorder = GetSystemMetrics(SM_CYBORDER);
    dac.dyCaption = GetSystemMetrics(SM_CYCAPTION);
    dac.dxVScroll = GetSystemMetrics(SM_CXVSCROLL);
    dac.dxSysFontChar = 8;
    dac.dySysFontChar = 16;
    dac.dySysFontAscent = 12;
    dac.clrWindow = GetSysColor(COLOR_WINDOW);
    dac.clrWindowFrame = GetSysColor(COLOR_WINDOWFRAME);
    dac.clrWindowText = GetSysColor(COLOR_WINDOWTEXT);
    dac.clrButton = GetSysColor(COLOR_BTNFACE);
    dac.clrButtonText = GetSysColor(COLOR_BTNTEXT);
    return true;
}

void EndSdm() {
    for (auto& entry : g_dialogs) {
        if (entry.second.window != nullptr && IsWindow(entry.second.window)) {
            DestroyWindow(entry.second.window);
        }
    }
    g_dialogs.clear();
    g_no_dialog = {};
    g_current_dialog = 0;
    g_focus_dialog = 0;
    g_initialized = false;
    sync_current_dialog_globals();
}

Word FtmeIsSdmMessage(MSG*) { return 0; }
void ChangeColors() {}

Hdlg HdlgStartDlg(DltHeader** dialog_template, Hcab cab, Dli* initializer) {
    Hdlg handle = 0;
    for (unsigned attempt = 0; attempt < 0xfffeu; ++attempt) {
        const Hdlg candidate = g_next_dialog;
        ++g_next_dialog;
        if (g_next_dialog == 0 || g_next_dialog == static_cast<Hdlg>(-1)) {
            g_next_dialog = 1;
        }
        if (g_dialogs.find(candidate) == g_dialogs.end()) {
            handle = candidate;
            break;
        }
    }
    if (handle == 0) {
        return 0;
    }

    DialogState dialog{};
    dialog.handle = handle;
    dialog.template_handle = dialog_template;
    dialog.cab = cab;
    if (dialog_template != nullptr && *dialog_template != nullptr) {
        dialog.hid = (*dialog_template)->hid;
        dialog.focus = (*dialog_template)->tmc_sel_init;
    }
    if (initializer != nullptr) {
        dialog.reference = initializer->reference;
        dialog.visible = true;
        dialog.modal = (initializer->flags & 0x00000001u) != 0;
    }
    dialog.window = create_dialog_host(dialog, initializer);
    g_dialogs.emplace(handle, std::move(dialog));
    materialize_icon_bar_template(g_dialogs.at(handle));
    g_current_dialog = handle;
    g_focus_dialog = handle;
    sync_current_dialog_globals();
    return handle;
}

Tmc TmcDoDlgDli(DltHeader** dialog_template, Hcab cab, Dli* initializer) {
    if (HdlgStartDlg(dialog_template, cab, initializer) == 0) {
        return static_cast<Tmc>(-1);
    }
    g_dialog.visible = true;
    return 2;  // tmcCancel until native template materialization is active.
}

Tmc TmcDoDlg_sdm21(DltHeader** dialog_template, Hcab cab,
                    unsigned char* runtime_items) {
    Dli initializer{};
    initializer.flags = 0x00000001u;
    initializer.runtime_items = runtime_items;
    return TmcDoDlgDli(dialog_template, cab, &initializer);
}

void EndDlg(Tmc) {
    g_dialog.dying = true;
    g_dialog.visible = false;
    if (g_dialog.window != nullptr && IsWindow(g_dialog.window)) {
        ShowWindow(g_dialog.window, SW_HIDE);
    }
}

int FFreeDlg() {
    const Hdlg handle = g_current_dialog;
    auto found = g_dialogs.find(handle);
    if (found == g_dialogs.end()) {
        return false;
    }
    if (found->second.window != nullptr && IsWindow(found->second.window)) {
        DestroyWindow(found->second.window);
    }
    g_dialogs.erase(found);
    if (g_focus_dialog == handle) {
        g_focus_dialog = 0;
    }
    g_current_dialog = 0;
    sync_current_dialog_globals();
    return true;
}

Word HidOfDlg(Hdlg dialog) {
    const auto* state = find_dialog(dialog == 0 ? g_current_dialog : dialog);
    return state == nullptr ? 0 : state->hid;
}

Hdlg HdlgSetCurDlg(Hdlg dialog) {
    const Hdlg previous = g_current_dialog;
    if (dialog == 0 || find_dialog(dialog) != nullptr) {
        g_current_dialog = dialog;
        sync_current_dialog_globals();
    }
    return previous;
}

Hdlg HdlgSetFocusDlg(Hdlg dialog) {
    const Hdlg previous = g_focus_dialog;
    if (dialog == 0 || find_dialog(dialog) != nullptr) {
        g_focus_dialog = dialog;
        sync_current_dialog_globals();
    }
    return previous;
}

int FKillDlgFocus() {
    HdlgSetFocusDlg(0);
    return true;
}

int FModalDlg(Hdlg dialog) {
    const auto* state = find_dialog(dialog);
    return state != nullptr && state->modal;
}

int FIsDlgDying() { return g_dialog.dying; }
void ClearListError(Hdlg) {}

void ShowDlg(int visible) {
    g_dialog.visible = visible != 0;
    if (g_dialog.window != nullptr && IsWindow(g_dialog.window)) {
        ShowWindow(g_dialog.window, visible ? SW_SHOWNA : SW_HIDE);
    }
}
int FVisibleDlg() { return g_dialog.visible; }
void ResizeDlg(int width, int height) {
    if (g_dialog.template_handle != nullptr &&
        *g_dialog.template_handle != nullptr) {
        (*g_dialog.template_handle)->rec.dx = width;
        (*g_dialog.template_handle)->rec.dy = height;
    }
    if (g_dialog.window != nullptr && IsWindow(g_dialog.window)) {
        SetWindowPos(g_dialog.window, nullptr, 0, 0, (std::max)(1, width),
                     (std::max)(1, height),
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}
void MoveDlg(int x, int y) {
    if (g_dialog.template_handle != nullptr &&
        *g_dialog.template_handle != nullptr) {
        (*g_dialog.template_handle)->rec.x = x;
        (*g_dialog.template_handle)->rec.y = y;
    }
    if (g_dialog.window != nullptr && IsWindow(g_dialog.window)) {
        SetWindowPos(g_dialog.window, nullptr, x, y, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}
void SdmScaleRec(Rec*) {}

int FSetDlgSab(Word sab) {
    g_dialog.sab = sab;
    return true;
}
Word SabGetDlg() { return g_dialog.sab; }

void SetTmcVal_sdm21(Tmc tmc, Word value) {
    auto& state = control(tmc);
    state.value = value;
    if (state.window != nullptr && IsWindow(state.window)) {
        SendMessageA(state.window, CB_SETCURSEL, value, 0);
    }
}
Word ValGetTmc(Tmc tmc) {
    const auto* state = find_control(tmc);
    return state == nullptr ? 0 : state->value;
}

void SetTmcText_sdm21(Tmc tmc, char* text) {
    auto& state = control(tmc);
    state.text = counted_or_zero_terminated(text);
    if (state.text.size() > state.text_limit) {
        state.text.resize(state.text_limit);
    }
    if (state.window != nullptr && IsWindow(state.window)) {
        SetWindowTextA(state.window, state.text.c_str());
    }
}
void GetTmcText_sdm21(Tmc tmc, char* destination, Word capacity) {
    const auto* state = find_control(tmc);
    copy_text(state == nullptr ? std::string{} : state->text, destination,
              capacity);
}
Word CchGetTmcText(Tmc tmc, char* destination, Word capacity) {
    const auto* state = find_control(tmc);
    const std::string empty;
    const auto& text = state == nullptr ? empty : state->text;
    copy_text(text, destination, capacity);
    return static_cast<Word>((std::min)(
        text.size(), static_cast<std::size_t>(0xffffu)));
}
Word CchGetTmc(Tmc tmc) { return CchGetTmcText(tmc, nullptr, 0); }

void GetTmcLargeVal(Tmc tmc, void* destination, Word byte_count) {
    if (destination == nullptr) {
        return;
    }
    const auto* state = find_control(tmc);
    const std::size_t count = state == nullptr
                                  ? 0
                                  : (std::min)(state->large_value.size(),
                                               std::size_t{byte_count});
    if (count != 0) {
        std::memcpy(destination, state->large_value.data(), count);
    }
    if (count < byte_count) {
        std::memset(static_cast<unsigned char*>(destination) + count, 0,
                    byte_count - count);
    }
}
int FSetTmcLargeVal(Tmc tmc, void* source) {
    if (source == nullptr) {
        return false;
    }
    /* Large values are application records; retain one pointer-sized unit
       when SDM has no generated TM metadata describing a larger record. */
    auto& value = control(tmc).large_value;
    value.resize(sizeof(void*));
    std::memcpy(value.data(), source, value.size());
    return true;
}

void SetFocusTmc(Tmc tmc) {
    g_dialog.focus = tmc;
    if (HWND window = ensure_control_window(tmc); window != nullptr) {
        SetFocus(window);
    }
}
Tmc TmcGetFocus() { return g_dialog.focus; }
void SetDefaultTmc(Tmc tmc) { g_dialog.default_tmc = tmc; }
Tmc TmcGetDefault(int) { return g_dialog.default_tmc; }
void SetTmcTxs(Tmc tmc, Dword selection) {
    control(tmc).selection = selection;
}
Dword TxsGetTmc(Tmc tmc) {
    const auto* state = find_control(tmc);
    return state == nullptr ? 0 : state->selection;
}
Word TmvGetTmc(Tmc tmc) {
    const auto* state = find_control(tmc);
    return state != nullptr && !state->text.empty() ? 3 : 1;
}
void RedisplayTmc(Tmc) {}

void EnableTmc_sdm21(Tmc tmc, int enabled) {
    auto& state = control(tmc);
    state.enabled = enabled != 0;
    if (state.window != nullptr && IsWindow(state.window)) {
        EnableWindow(state.window, state.enabled);
    }
}
int FEnabledTmc_sdm21(Tmc tmc) {
    const auto* state = find_control(tmc);
    return state == nullptr || state->enabled;
}
void EnableNoninteractiveTmc(Tmc tmc, int enabled) {
    EnableTmc_sdm21(tmc, enabled);
}
void SetVisibleTmc(Tmc tmc, int visible) {
    auto& state = control(tmc);
    state.visible = visible != 0;
    if (state.window != nullptr && IsWindow(state.window)) {
        ShowWindow(state.window, state.visible ? SW_SHOWNA : SW_HIDE);
    }
}
int FIsVisibleTmc(Tmc tmc) {
    const auto* state = find_control(tmc);
    return state == nullptr || state->visible;
}
void LimitTextTmc(Tmc tmc, Word limit) {
    auto& state = control(tmc);
    state.text_limit = limit;
    if (state.text.size() > limit) {
        state.text.resize(limit);
    }
}
void CompleteComboTmc(Tmc) {}

void AddListBoxEntry(Tmc tmc, char* entry) {
    auto& state = control(tmc);
    state.entries.emplace_back(counted_or_zero_terminated(entry));
    if (state.window != nullptr && IsWindow(state.window)) {
        SendMessageA(state.window, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(state.entries.back().c_str()));
    }
}
void InsertListBoxEntry(Tmc tmc, char* entry, Word index) {
    auto& state = control(tmc);
    auto& entries = state.entries;
    const auto position = (std::min)(entries.size(), std::size_t{index});
    entries.insert(entries.begin() + static_cast<std::ptrdiff_t>(position),
                   counted_or_zero_terminated(entry));
    if (state.window != nullptr && IsWindow(state.window)) {
        SendMessageA(state.window, CB_INSERTSTRING,
                     static_cast<WPARAM>(position),
                     reinterpret_cast<LPARAM>(entries[position].c_str()));
    }
}
void DeleteListBoxEntry(Tmc tmc, Word index) {
    auto& state = control(tmc);
    auto& entries = state.entries;
    if (index < entries.size()) {
        entries.erase(entries.begin() + index);
        if (state.window != nullptr && IsWindow(state.window)) {
            SendMessageA(state.window, CB_DELETESTRING, index, 0);
        }
    }
}
Word CentryListBoxTmc(Tmc tmc) {
    const auto* state = find_control(tmc);
    return static_cast<Word>(state == nullptr
                                 ? 0
                                 : (std::min)(state->entries.size(),
                                              std::size_t{0xffffu}));
}
Word CEntryListBoxTmc(Tmc tmc) { return CentryListBoxTmc(tmc); }
void GetListBoxEntry(Tmc tmc, Word index, char* destination, Word capacity) {
    const auto* state = find_control(tmc);
    copy_text(state == nullptr || index >= state->entries.size()
                  ? std::string{}
                  : state->entries[index],
              destination, capacity);
}
Word CchGetListBoxEntry(Tmc tmc, Word index, char* destination,
                        Word capacity) {
    const auto* state = find_control(tmc);
    const std::string empty;
    const auto& entry = state == nullptr || index >= state->entries.size()
                            ? empty
                            : state->entries[index];
    copy_text(entry, destination, capacity);
    return static_cast<Word>((std::min)(entry.size(), std::size_t{0xffffu}));
}
Word IEntryFindListBox(Tmc tmc, char* entry, Word*) {
    const auto* state = find_control(tmc);
    if (state == nullptr) {
        return static_cast<Word>(-1);
    }
    const std::string sought = counted_or_zero_terminated(entry);
    const auto found = std::find(state->entries.begin(), state->entries.end(),
                                 sought);
    return found == state->entries.end()
               ? static_cast<Word>(-1)
               : static_cast<Word>(found - state->entries.begin());
}
Word CselListBoxTmc(Tmc tmc) {
    const auto* state = find_control(tmc);
    return state == nullptr || state->entries.empty() ? 0 : 1;
}
void StartListBoxUpdate(Tmc) {}
void BeginListBoxUpdate(Tmc, int) {}
void EndListBoxUpdate(Tmc) {}

Hcab HcabFromDlg(int dialog) {
    const auto* state =
        find_dialog(dialog == 0 ? g_current_dialog : static_cast<Hdlg>(dialog));
    return state == nullptr ? nullptr : state->cab;
}
void SaveCabs(void (*callback)(Hcab, Word, Tmc, int), int save) {
    if (callback != nullptr && g_dialog.cab != nullptr) {
        callback(g_dialog.cab, g_dialog.reference, 0, save);
    }
}

void GetTmcRec(Tmc tmc, Rec* rectangle) {
    if (rectangle == nullptr) {
        return;
    }
    const auto* state = find_control(tmc);
    *rectangle = state == nullptr ? Rec{} : state->rectangle;
}
HWND HwndOfTmc(Tmc tmc) { return ensure_control_window(tmc); }
Hdlg HdlgFromHwnd(HWND window) {
    if (window == nullptr) {
        return 0;
    }
    for (const auto& entry : g_dialogs) {
        if (entry.second.window == window ||
            (entry.second.window != nullptr &&
             IsChild(entry.second.window, window))) {
            return entry.first;
        }
    }
    return 0;
}
HWND HwndFromDlg(Hdlg dialog) {
    const auto* state = find_dialog(dialog == 0 ? g_current_dialog : dialog);
    return state == nullptr ? nullptr : state->window;
}
HWND HwndSwapSdmParent(HWND window) {
    return std::exchange(vhWndMsgBoxParent, window);
}
int FIsDlgInteractive() { return !g_noninteractive; }
void SetDlgCaption(char* caption) {
    g_dialog.caption = counted_or_zero_terminated(caption);
    if (g_dialog.window != nullptr && IsWindow(g_dialog.window)) {
        SetWindowTextA(g_dialog.window, g_dialog.caption.c_str());
    }
}

int FSetNoninteractive(Word, Tmc) {
    g_noninteractive = true;
    return true;
}
void EndSdmTranscription() { g_noninteractive = false; }
int FExecutable() { return true; }
void CBTState(int) {}
int FRestoreDlg(int) { return true; }
int FRestoreTmc(Tmc, int) { return true; }

Word IdDoMsgBox(char* text, char* caption, Word flags) {
    return static_cast<Word>(MessageBoxA(vhWndMsgBoxParent, text, caption,
                                         static_cast<UINT>(flags)));
}

Word FlbfFillDirListTmc(char*, char*, Tmc, Tmc, Tmc, Word, Word) {
    return 0x0008;  // flbfListingMade; directory enumeration is UI-backed.
}

}  // extern "C"

namespace {

void set_sds_handle(const Hdlg current, const Hdlg focus) {
    sds.current_dialog =
        reinterpret_cast<void**>(static_cast<std::uintptr_t>(current));
    sds.focus_dialog =
        reinterpret_cast<void**>(static_cast<std::uintptr_t>(focus));
}

}  // namespace
