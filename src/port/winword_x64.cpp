#include "legacy_asm_port.h"

#include <Windows.h>
#include <CommCtrl.h>
#include <CommDlg.h>
#include <Richedit.h>
#include <Shellapi.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <cwctype>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr wchar_t kWindowClass[] = L"OpusWordX64Window";
constexpr wchar_t kProductName[] = L"Microsoft Word x64 Port";
constexpr UINT_PTR kStatusTimer = 1;

enum Command : UINT {
    cmd_file_new = 100,
    cmd_file_open,
    cmd_file_save,
    cmd_file_save_as,
    cmd_file_print,
    cmd_file_exit,
    cmd_edit_undo = 200,
    cmd_edit_redo,
    cmd_edit_cut,
    cmd_edit_copy,
    cmd_edit_paste,
    cmd_edit_select_all,
    cmd_edit_find,
    cmd_edit_find_next,
    cmd_format_bold = 300,
    cmd_format_italic,
    cmd_format_underline,
    cmd_format_font,
    cmd_format_left,
    cmd_format_center,
    cmd_format_right,
    cmd_format_justify,
    cmd_tools_word_count = 400,
    cmd_help_about = 500,
};

[[nodiscard]] std::wstring system_error_message(const DWORD error) {
    wchar_t* buffer = nullptr;
    const DWORD count = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error, 0, reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);
    std::wstring message = count != 0 && buffer != nullptr
                               ? std::wstring(buffer, count)
                               : L"Unknown error (" + std::to_wstring(error) + L")";
    if (buffer != nullptr) {
        LocalFree(buffer);
    }
    while (!message.empty() && (message.back() == L'\r' || message.back() == L'\n')) {
        message.pop_back();
    }
    return message;
}

[[nodiscard]] std::wstring lower_extension(const std::wstring& path) {
    auto extension = std::filesystem::path(path).extension().wstring();
    std::ranges::transform(extension, extension.begin(),
                           [](const wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); });
    return extension;
}

[[nodiscard]] std::vector<std::byte> read_file(const std::wstring& path) {
    const HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("CreateFileW failed: " + std::to_string(GetLastError()));
    }

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart < 0 ||
        size.QuadPart > static_cast<LONGLONG>(std::numeric_limits<DWORD>::max())) {
        const DWORD error = GetLastError();
        CloseHandle(file);
        throw std::runtime_error("GetFileSizeEx failed: " + std::to_string(error));
    }

    std::vector<std::byte> bytes(static_cast<std::size_t>(size.QuadPart));
    DWORD read = 0;
    const BOOL succeeded = bytes.empty() ||
                           ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr);
    const DWORD error = succeeded ? ERROR_SUCCESS : GetLastError();
    CloseHandle(file);
    if (!succeeded || read != bytes.size()) {
        throw std::runtime_error("ReadFile failed: " + std::to_string(error));
    }
    return bytes;
}

void write_file(const std::wstring& path, const std::vector<std::byte>& bytes) {
    const HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("CreateFileW failed: " + std::to_string(GetLastError()));
    }
    DWORD written = 0;
    const BOOL succeeded = bytes.empty() ||
                           WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr);
    const DWORD error = succeeded ? ERROR_SUCCESS : GetLastError();
    CloseHandle(file);
    if (!succeeded || written != bytes.size()) {
        throw std::runtime_error("WriteFile failed: " + std::to_string(error));
    }
}

[[nodiscard]] std::wstring decode_text(const std::vector<std::byte>& bytes) {
    if (bytes.size() >= 2 && bytes[0] == std::byte{0xff} && bytes[1] == std::byte{0xfe}) {
        std::wstring result((bytes.size() - 2) / sizeof(wchar_t), L'\0');
        std::memcpy(result.data(), bytes.data() + 2, result.size() * sizeof(wchar_t));
        return result;
    }

    std::size_t offset = 0;
    if (bytes.size() >= 3 && bytes[0] == std::byte{0xef} && bytes[1] == std::byte{0xbb} &&
        bytes[2] == std::byte{0xbf}) {
        offset = 3;
    }
    const auto* text = reinterpret_cast<const char*>(bytes.data() + offset);
    const int byte_count = static_cast<int>(bytes.size() - offset);
    if (byte_count == 0) {
        return {};
    }

    UINT code_page = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    int character_count = MultiByteToWideChar(code_page, flags, text, byte_count, nullptr, 0);
    if (character_count == 0) {
        code_page = CP_ACP;
        flags = 0;
        character_count = MultiByteToWideChar(code_page, flags, text, byte_count, nullptr, 0);
    }
    if (character_count == 0) {
        throw std::runtime_error("The text encoding could not be decoded");
    }
    std::wstring result(static_cast<std::size_t>(character_count), L'\0');
    MultiByteToWideChar(code_page, flags, text, byte_count, result.data(), character_count);
    return result;
}

[[nodiscard]] std::vector<std::byte> encode_utf8(const std::wstring_view text) {
    const int byte_count = WideCharToMultiByte(CP_UTF8, 0, text.data(),
                                               static_cast<int>(text.size()), nullptr, 0,
                                               nullptr, nullptr);
    std::vector<std::byte> result{
        std::byte{0xef}, std::byte{0xbb}, std::byte{0xbf},
    };
    const auto initial_size = result.size();
    result.resize(initial_size + static_cast<std::size_t>(byte_count));
    if (byte_count != 0) {
        WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                            reinterpret_cast<char*>(result.data() + initial_size), byte_count,
                            nullptr, nullptr);
    }
    return result;
}

[[nodiscard]] HMENU build_menu() {
    const HMENU menu = CreateMenu();
    const HMENU file = CreatePopupMenu();
    AppendMenuW(file, MF_STRING, cmd_file_new, L"&New\tCtrl+N");
    AppendMenuW(file, MF_STRING, cmd_file_open, L"&Open...\tCtrl+O");
    AppendMenuW(file, MF_STRING, cmd_file_save, L"&Save\tCtrl+S");
    AppendMenuW(file, MF_STRING, cmd_file_save_as, L"Save &As...");
    AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(file, MF_STRING, cmd_file_print, L"&Print...\tCtrl+P");
    AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(file, MF_STRING, cmd_file_exit, L"E&xit");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(file), L"&File");

    const HMENU edit = CreatePopupMenu();
    AppendMenuW(edit, MF_STRING, cmd_edit_undo, L"&Undo\tCtrl+Z");
    AppendMenuW(edit, MF_STRING, cmd_edit_redo, L"&Redo\tCtrl+Y");
    AppendMenuW(edit, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(edit, MF_STRING, cmd_edit_cut, L"Cu&t\tCtrl+X");
    AppendMenuW(edit, MF_STRING, cmd_edit_copy, L"&Copy\tCtrl+C");
    AppendMenuW(edit, MF_STRING, cmd_edit_paste, L"&Paste\tCtrl+V");
    AppendMenuW(edit, MF_STRING, cmd_edit_select_all, L"Select &All\tCtrl+A");
    AppendMenuW(edit, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(edit, MF_STRING, cmd_edit_find, L"&Find...\tCtrl+F");
    AppendMenuW(edit, MF_STRING, cmd_edit_find_next, L"Find &Next\tF3");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(edit), L"&Edit");

    const HMENU format = CreatePopupMenu();
    AppendMenuW(format, MF_STRING, cmd_format_bold, L"&Bold\tCtrl+B");
    AppendMenuW(format, MF_STRING, cmd_format_italic, L"&Italic\tCtrl+I");
    AppendMenuW(format, MF_STRING, cmd_format_underline, L"&Underline\tCtrl+U");
    AppendMenuW(format, MF_STRING, cmd_format_font, L"&Font...");
    AppendMenuW(format, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(format, MF_STRING, cmd_format_left, L"Align &Left\tCtrl+L");
    AppendMenuW(format, MF_STRING, cmd_format_center, L"&Center\tCtrl+E");
    AppendMenuW(format, MF_STRING, cmd_format_right, L"Align &Right\tCtrl+R");
    AppendMenuW(format, MF_STRING, cmd_format_justify, L"&Justify\tCtrl+J");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(format), L"F&ormat");

    const HMENU tools = CreatePopupMenu();
    AppendMenuW(tools, MF_STRING, cmd_tools_word_count, L"&Word Count...");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(tools), L"&Tools");

    const HMENU help = CreatePopupMenu();
    AppendMenuW(help, MF_STRING, cmd_help_about, L"&About Microsoft Word x64 Port");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(help), L"&Help");
    return menu;
}

class WordApplication {
public:
    explicit WordApplication(const HINSTANCE instance) : instance_(instance) {}

    int run(const int show_command) {
        int argument_count = 0;
        wchar_t** arguments = CommandLineToArgvW(GetCommandLineW(), &argument_count);
        if (arguments != nullptr && argument_count > 1 &&
            std::wstring_view(arguments[1]) == L"--self-test") {
            std::array<std::uint8_t, 7> pattern_buffer{};
            constexpr std::array<std::uint8_t, 3> pattern{1, 2, 3};
            opus::x64::fill_byte_pattern(pattern_buffer.data(), pattern_buffer.size(),
                                         pattern.data(), pattern.size());
            const bool passed = opus::x64::assembly_manifest_complete() &&
                                opus::x64::assembly_manifest_count() == 59 &&
                                opus::x64::multiply_divide(10, 3, 2) == 15 &&
                                opus::x64::legacy_square(4.0) == 16.0 &&
                                pattern_buffer == std::array<std::uint8_t, 7>{1, 2, 3, 1, 2, 3, 1} &&
                                opus::x64::is_ascii_alphanumeric('Z') &&
                                opus::x64::ascii_upper('q') == 'Q';
            LocalFree(arguments);
            return passed ? 0 : 2;
        }

        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_BAR_CLASSES | ICC_STANDARD_CLASSES};
        InitCommonControlsEx(&controls);

        rich_edit_library_ = LoadLibraryW(L"Msftedit.dll");
        if (rich_edit_library_ == nullptr) {
            show_error(L"Microsoft Rich Edit could not be loaded", GetLastError());
            if (arguments != nullptr) {
                LocalFree(arguments);
            }
            return 1;
        }

        WNDCLASSEXW window_class{sizeof(window_class)};
        window_class.style = CS_HREDRAW | CS_VREDRAW;
        window_class.lpfnWndProc = &WordApplication::window_proc;
        window_class.hInstance = instance_;
        window_class.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_APPWORKSPACE + 1);
        window_class.lpszClassName = kWindowClass;
        window_class.hIconSm = window_class.hIcon;
        if (RegisterClassExW(&window_class) == 0) {
            show_error(L"The application window could not be registered", GetLastError());
            FreeLibrary(rich_edit_library_);
            if (arguments != nullptr) {
                LocalFree(arguments);
            }
            return 1;
        }

        main_window_ = CreateWindowExW(
            0, kWindowClass, kProductName, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
            CW_USEDEFAULT, CW_USEDEFAULT, 1100, 800, nullptr, nullptr, instance_, this);
        if (main_window_ == nullptr) {
            show_error(L"The application window could not be created", GetLastError());
            FreeLibrary(rich_edit_library_);
            if (arguments != nullptr) {
                LocalFree(arguments);
            }
            return 1;
        }

        if (arguments != nullptr && argument_count > 1 &&
            std::wstring_view(arguments[1]) == L"--ui-self-test") {
            SetWindowTextW(editor_, L"Word x64 UI smoke test");
            const bool passed = IsWindow(main_window_) != FALSE && IsWindow(editor_) != FALSE &&
                                IsWindow(status_) != FALSE && GetWindowTextLengthW(editor_) == 22;
            DestroyWindow(main_window_);
            LocalFree(arguments);
            FreeLibrary(rich_edit_library_);
            return passed ? 0 : 3;
        }

        if (arguments != nullptr && argument_count > 1) {
            open_path(arguments[1]);
        }
        if (arguments != nullptr) {
            LocalFree(arguments);
        }

        ShowWindow(main_window_, show_command);
        UpdateWindow(main_window_);

        const std::array accelerators{
            ACCEL{FVIRTKEY | FCONTROL, 'N', cmd_file_new},
            ACCEL{FVIRTKEY | FCONTROL, 'O', cmd_file_open},
            ACCEL{FVIRTKEY | FCONTROL, 'S', cmd_file_save},
            ACCEL{FVIRTKEY | FCONTROL, 'P', cmd_file_print},
            ACCEL{FVIRTKEY | FCONTROL, 'Z', cmd_edit_undo},
            ACCEL{FVIRTKEY | FCONTROL, 'Y', cmd_edit_redo},
            ACCEL{FVIRTKEY | FCONTROL, 'X', cmd_edit_cut},
            ACCEL{FVIRTKEY | FCONTROL, 'C', cmd_edit_copy},
            ACCEL{FVIRTKEY | FCONTROL, 'V', cmd_edit_paste},
            ACCEL{FVIRTKEY | FCONTROL, 'A', cmd_edit_select_all},
            ACCEL{FVIRTKEY | FCONTROL, 'F', cmd_edit_find},
            ACCEL{FVIRTKEY, VK_F3, cmd_edit_find_next},
            ACCEL{FVIRTKEY | FCONTROL, 'B', cmd_format_bold},
            ACCEL{FVIRTKEY | FCONTROL, 'I', cmd_format_italic},
            ACCEL{FVIRTKEY | FCONTROL, 'U', cmd_format_underline},
            ACCEL{FVIRTKEY | FCONTROL, 'L', cmd_format_left},
            ACCEL{FVIRTKEY | FCONTROL, 'E', cmd_format_center},
            ACCEL{FVIRTKEY | FCONTROL, 'R', cmd_format_right},
            ACCEL{FVIRTKEY | FCONTROL, 'J', cmd_format_justify},
        };
        const HACCEL accelerator_table = CreateAcceleratorTableW(
            const_cast<LPACCEL>(accelerators.data()), static_cast<int>(accelerators.size()));

        MSG message{};
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            if (find_dialog_ != nullptr && IsDialogMessageW(find_dialog_, &message)) {
                continue;
            }
            if (!TranslateAcceleratorW(main_window_, accelerator_table, &message)) {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }
        DestroyAcceleratorTable(accelerator_table);
        FreeLibrary(rich_edit_library_);
        return static_cast<int>(message.wParam);
    }

private:
    static LRESULT CALLBACK window_proc(const HWND window, const UINT message,
                                        const WPARAM wparam, const LPARAM lparam) {
        WordApplication* app = reinterpret_cast<WordApplication*>(
            GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
            app = static_cast<WordApplication*>(create->lpCreateParams);
            app->main_window_ = window;
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        }
        return app != nullptr ? app->dispatch(message, wparam, lparam)
                              : DefWindowProcW(window, message, wparam, lparam);
    }

    LRESULT dispatch(const UINT message, const WPARAM wparam, const LPARAM lparam) {
        if (message == find_message_) {
            on_find_message(*reinterpret_cast<const FINDREPLACEW*>(lparam));
            return 0;
        }
        switch (message) {
            case WM_CREATE:
                return create_children() ? 0 : -1;
            case WM_SIZE:
                layout(LOWORD(lparam), HIWORD(lparam));
                return 0;
            case WM_SETFOCUS:
                SetFocus(editor_);
                return 0;
            case WM_COMMAND:
                if (reinterpret_cast<HWND>(lparam) == editor_ && HIWORD(wparam) == EN_CHANGE) {
                    status_dirty_ = true;
                    update_title();
                    return 0;
                }
                execute_command(LOWORD(wparam));
                return 0;
            case WM_NOTIFY:
                if (reinterpret_cast<const NMHDR*>(lparam)->hwndFrom == editor_) {
                    status_dirty_ = true;
                }
                return 0;
            case WM_INITMENU:
                update_menu(reinterpret_cast<HMENU>(wparam));
                return 0;
            case WM_TIMER:
                if (wparam == kStatusTimer && status_dirty_) {
                    update_status();
                }
                return 0;
            case WM_DROPFILES:
                open_drop(reinterpret_cast<HDROP>(wparam));
                return 0;
            case WM_CLOSE:
                if (prompt_save_changes()) {
                    DestroyWindow(main_window_);
                }
                return 0;
            case WM_DESTROY:
                KillTimer(main_window_, kStatusTimer);
                PostQuitMessage(0);
                return 0;
            default:
                return DefWindowProcW(main_window_, message, wparam, lparam);
        }
    }

    bool create_children() {
        SetMenu(main_window_, build_menu());
        editor_ = CreateWindowExW(
            WS_EX_CLIENTEDGE, MSFTEDIT_CLASS, L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_WANTRETURN |
                ES_NOHIDESEL | ES_AUTOVSCROLL,
            0, 0, 0, 0, main_window_, reinterpret_cast<HMENU>(1), instance_, nullptr);
        status_ = CreateWindowExW(0, STATUSCLASSNAMEW, nullptr, WS_CHILD | WS_VISIBLE,
                                  0, 0, 0, 0, main_window_, reinterpret_cast<HMENU>(2),
                                  instance_, nullptr);
        if (editor_ == nullptr || status_ == nullptr) {
            return false;
        }

        SendMessageW(editor_, EM_SETEVENTMASK, 0, ENM_CHANGE | ENM_SELCHANGE);
        SendMessageW(editor_, EM_SETBKGNDCOLOR, 0, RGB(255, 255, 255));
        SendMessageW(editor_, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(16, 16));
        SendMessageW(editor_, EM_EXLIMITTEXT, 0, std::numeric_limits<LONG>::max());

        CHARFORMAT2W format{sizeof(format)};
        format.dwMask = CFM_FACE | CFM_SIZE | CFM_CHARSET;
        format.yHeight = 220;
        format.bCharSet = DEFAULT_CHARSET;
        wcscpy_s(format.szFaceName, L"Segoe UI");
        SendMessageW(editor_, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&format));

        find_message_ = RegisterWindowMessageW(FINDMSGSTRINGW);
        DragAcceptFiles(main_window_, TRUE);
        SetTimer(main_window_, kStatusTimer, 250, nullptr);
        set_clean();
        update_status();
        SetFocus(editor_);
        return true;
    }

    void layout(const int width, const int height) {
        SendMessageW(status_, WM_SIZE, 0, 0);
        RECT status_rect{};
        GetWindowRect(status_, &status_rect);
        const int status_height = status_rect.bottom - status_rect.top;
        MoveWindow(editor_, 0, 0, width, std::max(0, height - status_height), TRUE);
        const int parts[]{std::max(200, width - 330), std::max(320, width - 150), -1};
        SendMessageW(status_, SB_SETPARTS, std::size(parts), reinterpret_cast<LPARAM>(parts));
        status_dirty_ = true;
    }

    void execute_command(const UINT command) {
        switch (command) {
            case cmd_file_new: new_document(); break;
            case cmd_file_open: open_dialog(); break;
            case cmd_file_save: save(false); break;
            case cmd_file_save_as: save(true); break;
            case cmd_file_print: print_document(); break;
            case cmd_file_exit: SendMessageW(main_window_, WM_CLOSE, 0, 0); break;
            case cmd_edit_undo: SendMessageW(editor_, EM_UNDO, 0, 0); break;
            case cmd_edit_redo: SendMessageW(editor_, EM_REDO, 0, 0); break;
            case cmd_edit_cut: SendMessageW(editor_, WM_CUT, 0, 0); break;
            case cmd_edit_copy: SendMessageW(editor_, WM_COPY, 0, 0); break;
            case cmd_edit_paste: SendMessageW(editor_, WM_PASTE, 0, 0); break;
            case cmd_edit_select_all: SendMessageW(editor_, EM_SETSEL, 0, -1); break;
            case cmd_edit_find: show_find(); break;
            case cmd_edit_find_next: find_next(find_flags_); break;
            case cmd_format_bold: toggle_effect(CFM_BOLD, CFE_BOLD); break;
            case cmd_format_italic: toggle_effect(CFM_ITALIC, CFE_ITALIC); break;
            case cmd_format_underline: toggle_effect(CFM_UNDERLINE, CFE_UNDERLINE); break;
            case cmd_format_font: choose_font(); break;
            case cmd_format_left: set_alignment(PFA_LEFT); break;
            case cmd_format_center: set_alignment(PFA_CENTER); break;
            case cmd_format_right: set_alignment(PFA_RIGHT); break;
            case cmd_format_justify: set_alignment(PFA_JUSTIFY); break;
            case cmd_tools_word_count: show_word_count(); break;
            case cmd_help_about: show_about(); break;
            default: break;
        }
        status_dirty_ = true;
    }

    void update_menu(const HMENU menu) const {
        const bool can_undo = SendMessageW(editor_, EM_CANUNDO, 0, 0) != 0;
        const bool can_redo = SendMessageW(editor_, EM_CANREDO, 0, 0) != 0;
        CHARRANGE selection{};
        SendMessageW(editor_, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&selection));
        EnableMenuItem(menu, cmd_edit_undo, MF_BYCOMMAND | (can_undo ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(menu, cmd_edit_redo, MF_BYCOMMAND | (can_redo ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(menu, cmd_edit_cut,
                       MF_BYCOMMAND | (selection.cpMin != selection.cpMax ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(menu, cmd_edit_copy,
                       MF_BYCOMMAND | (selection.cpMin != selection.cpMax ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(menu, cmd_edit_paste,
                       MF_BYCOMMAND | (SendMessageW(editor_, EM_CANPASTE, 0, 0) ? MF_ENABLED : MF_GRAYED));
    }

    void new_document() {
        if (!prompt_save_changes()) {
            return;
        }
        loading_ = true;
        SetWindowTextW(editor_, L"");
        current_path_.clear();
        loading_ = false;
        set_clean();
    }

    void open_dialog() {
        if (!prompt_save_changes()) {
            return;
        }
        std::array<wchar_t, 32768> path{};
        OPENFILENAMEW dialog{sizeof(dialog)};
        dialog.hwndOwner = main_window_;
        dialog.lpstrFilter = L"Rich Text Format (*.rtf)\0*.rtf\0Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0";
        dialog.lpstrFile = path.data();
        dialog.nMaxFile = static_cast<DWORD>(path.size());
        dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
        if (GetOpenFileNameW(&dialog)) {
            open_path(path.data());
        }
    }

    void open_drop(const HDROP drop) {
        std::array<wchar_t, 32768> path{};
        if (DragQueryFileW(drop, 0, path.data(), static_cast<UINT>(path.size())) != 0 &&
            prompt_save_changes()) {
            open_path(path.data());
        }
        DragFinish(drop);
    }

    void open_path(const std::wstring& path) {
        if (lower_extension(path) == L".doc") {
            MessageBoxW(main_window_,
                        L"The native x64 document engine does not read binary Word 1.x .doc files yet. "
                        L"Open an RTF or text document for this migration milestone.",
                        kProductName, MB_OK | MB_ICONINFORMATION);
            return;
        }
        try {
            loading_ = true;
            if (lower_extension(path) == L".rtf") {
                const HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (file == INVALID_HANDLE_VALUE) {
                    throw std::runtime_error("CreateFileW failed: " + std::to_string(GetLastError()));
                }
                EDITSTREAM stream{reinterpret_cast<DWORD_PTR>(file), 0, &stream_in};
                SetWindowTextW(editor_, L"");
                SendMessageW(editor_, EM_STREAMIN, SF_RTF, reinterpret_cast<LPARAM>(&stream));
                CloseHandle(file);
                if (stream.dwError != 0) {
                    throw std::runtime_error("Rich Edit could not read the RTF stream");
                }
            } else {
                const auto text = decode_text(read_file(path));
                SetWindowTextW(editor_, text.c_str());
            }
            current_path_ = path;
            loading_ = false;
            set_clean();
        } catch (const std::exception& error) {
            loading_ = false;
            const std::wstring detail(error.what(), error.what() + std::strlen(error.what()));
            MessageBoxW(main_window_, (L"Could not open the document.\n\n" + detail).c_str(),
                        kProductName, MB_OK | MB_ICONERROR);
        }
    }

    bool save(const bool save_as) {
        std::wstring path = current_path_;
        if (save_as || path.empty()) {
            std::array<wchar_t, 32768> selected{};
            if (!path.empty()) {
                wcsncpy_s(selected.data(), selected.size(), path.c_str(), _TRUNCATE);
            } else {
                wcscpy_s(selected.data(), selected.size(), L"Document.rtf");
            }
            OPENFILENAMEW dialog{sizeof(dialog)};
            dialog.hwndOwner = main_window_;
            dialog.lpstrFilter = L"Rich Text Format (*.rtf)\0*.rtf\0UTF-8 text (*.txt)\0*.txt\0";
            dialog.lpstrFile = selected.data();
            dialog.nMaxFile = static_cast<DWORD>(selected.size());
            dialog.lpstrDefExt = L"rtf";
            dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
            if (!GetSaveFileNameW(&dialog)) {
                return false;
            }
            path = selected.data();
        }

        try {
            if (lower_extension(path) == L".rtf") {
                const HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                                FILE_ATTRIBUTE_NORMAL, nullptr);
                if (file == INVALID_HANDLE_VALUE) {
                    throw std::runtime_error("CreateFileW failed: " + std::to_string(GetLastError()));
                }
                EDITSTREAM stream{reinterpret_cast<DWORD_PTR>(file), 0, &stream_out};
                SendMessageW(editor_, EM_STREAMOUT, SF_RTF, reinterpret_cast<LPARAM>(&stream));
                CloseHandle(file);
                if (stream.dwError != 0) {
                    throw std::runtime_error("Rich Edit could not write the RTF stream");
                }
            } else {
                write_file(path, encode_utf8(document_text()));
            }
            current_path_ = path;
            set_clean();
            return true;
        } catch (const std::exception& error) {
            const std::wstring detail(error.what(), error.what() + std::strlen(error.what()));
            MessageBoxW(main_window_, (L"Could not save the document.\n\n" + detail).c_str(),
                        kProductName, MB_OK | MB_ICONERROR);
            return false;
        }
    }

    bool prompt_save_changes() {
        if (SendMessageW(editor_, EM_GETMODIFY, 0, 0) == 0) {
            return true;
        }
        const int answer = MessageBoxW(main_window_, L"Save changes to this document?",
                                       kProductName, MB_YESNOCANCEL | MB_ICONQUESTION);
        return answer == IDNO || (answer == IDYES && save(false));
    }

    void set_clean() {
        SendMessageW(editor_, EM_SETMODIFY, FALSE, 0);
        update_title();
        status_dirty_ = true;
    }

    void update_title() const {
        std::wstring name = current_path_.empty()
                                ? L"Document"
                                : std::filesystem::path(current_path_).filename().wstring();
        if (!loading_ && SendMessageW(editor_, EM_GETMODIFY, 0, 0) != 0) {
            name += L" *";
        }
        SetWindowTextW(main_window_, (name + L" - " + kProductName).c_str());
    }

    [[nodiscard]] std::wstring document_text() const {
        const int length = GetWindowTextLengthW(editor_);
        std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');
        if (length != 0) {
            GetWindowTextW(editor_, text.data(), length + 1);
        }
        text.resize(static_cast<std::size_t>(length));
        return text;
    }

    [[nodiscard]] static std::pair<std::size_t, std::size_t> count_text(
        const std::wstring_view text) {
        std::size_t words = 0;
        std::size_t characters = 0;
        bool in_word = false;
        for (const wchar_t character : text) {
            if (character != L'\r' && character != L'\n') {
                ++characters;
            }
            const bool word_character = std::iswalnum(character) != 0 || character == L'_';
            if (word_character && !in_word) {
                ++words;
            }
            in_word = word_character;
        }
        return {words, characters};
    }

    void update_status() {
        const auto text = document_text();
        const auto [words, characters] = count_text(text);
        CHARRANGE selection{};
        SendMessageW(editor_, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&selection));
        const LRESULT line = SendMessageW(editor_, EM_LINEFROMCHAR, selection.cpMin, 0);
        const LRESULT line_start = SendMessageW(editor_, EM_LINEINDEX, line, 0);

        const std::wstring file = current_path_.empty() ? L"  New document" : L"  " + current_path_;
        const std::wstring counts = L"Words: " + std::to_wstring(words) +
                                    L"   Characters: " + std::to_wstring(characters);
        const std::wstring position = L"Ln " + std::to_wstring(line + 1) +
                                      L", Col " + std::to_wstring(selection.cpMin - line_start + 1);
        SendMessageW(status_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(file.c_str()));
        SendMessageW(status_, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(counts.c_str()));
        SendMessageW(status_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(position.c_str()));
        status_dirty_ = false;
    }

    void show_word_count() const {
        const auto [words, characters] = count_text(document_text());
        const std::wstring message = L"Words: " + std::to_wstring(words) +
                                     L"\nCharacters (excluding paragraph marks): " +
                                     std::to_wstring(characters);
        MessageBoxW(main_window_, message.c_str(), L"Word Count", MB_OK | MB_ICONINFORMATION);
    }

    void toggle_effect(const DWORD mask, const DWORD effect) const {
        CHARFORMAT2W format{sizeof(format)};
        format.dwMask = mask;
        SendMessageW(editor_, EM_GETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));
        format.dwEffects ^= effect;
        SendMessageW(editor_, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));
    }

    void set_alignment(const WORD alignment) const {
        PARAFORMAT2 format{sizeof(format)};
        format.dwMask = PFM_ALIGNMENT;
        format.wAlignment = alignment;
        SendMessageW(editor_, EM_SETPARAFORMAT, 0, reinterpret_cast<LPARAM>(&format));
    }

    void choose_font() const {
        CHARFORMAT2W current{sizeof(current)};
        SendMessageW(editor_, EM_GETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&current));
        HDC dc = GetDC(editor_);
        LOGFONTW font{};
        font.lfHeight = -MulDiv(current.yHeight / 20, GetDeviceCaps(dc, LOGPIXELSY), 72);
        font.lfWeight = (current.dwEffects & CFE_BOLD) != 0 ? FW_BOLD : FW_NORMAL;
        font.lfItalic = (current.dwEffects & CFE_ITALIC) != 0;
        font.lfUnderline = (current.dwEffects & CFE_UNDERLINE) != 0;
        wcsncpy_s(font.lfFaceName, current.szFaceName, _TRUNCATE);
        ReleaseDC(editor_, dc);

        CHOOSEFONTW dialog{sizeof(dialog)};
        dialog.hwndOwner = main_window_;
        dialog.lpLogFont = &font;
        dialog.iPointSize = current.yHeight / 2;
        dialog.rgbColors = current.crTextColor;
        dialog.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_EFFECTS;
        if (!ChooseFontW(&dialog)) {
            return;
        }

        CHARFORMAT2W selected{sizeof(selected)};
        selected.dwMask = CFM_FACE | CFM_SIZE | CFM_COLOR | CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE;
        selected.yHeight = dialog.iPointSize * 2;
        selected.crTextColor = dialog.rgbColors;
        selected.dwEffects = (font.lfWeight >= FW_BOLD ? CFE_BOLD : 0) |
                             (font.lfItalic ? CFE_ITALIC : 0) |
                             (font.lfUnderline ? CFE_UNDERLINE : 0);
        wcsncpy_s(selected.szFaceName, font.lfFaceName, _TRUNCATE);
        SendMessageW(editor_, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&selected));
    }

    void show_find() {
        if (find_dialog_ != nullptr) {
            SetForegroundWindow(find_dialog_);
            return;
        }
        find_state_ = {};
        find_state_.lStructSize = sizeof(find_state_);
        find_state_.hwndOwner = main_window_;
        find_state_.Flags = FR_DOWN;
        find_state_.lpstrFindWhat = find_text_.data();
        find_state_.wFindWhatLen = static_cast<WORD>(find_text_.size());
        find_dialog_ = FindTextW(&find_state_);
    }

    void on_find_message(const FINDREPLACEW& message) {
        if ((message.Flags & FR_DIALOGTERM) != 0) {
            find_dialog_ = nullptr;
            return;
        }
        if ((message.Flags & FR_FINDNEXT) != 0) {
            find_flags_ = message.Flags;
            find_next(message.Flags);
        }
    }

    void find_next(const DWORD flags) {
        if (find_text_[0] == L'\0') {
            show_find();
            return;
        }
        CHARRANGE selection{};
        SendMessageW(editor_, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&selection));
        const LONG text_length = GetWindowTextLengthW(editor_);
        FINDTEXTEXW search{};
        search.lpstrText = find_text_.data();
        const bool down = (flags & FR_DOWN) != 0;
        search.chrg = down ? CHARRANGE{selection.cpMax, text_length}
                           : CHARRANGE{selection.cpMin - 1, 0};
        DWORD options = down ? FR_DOWN : 0;
        options |= flags & (FR_MATCHCASE | FR_WHOLEWORD);
        LRESULT result = SendMessageW(editor_, EM_FINDTEXTEXW, options,
                                      reinterpret_cast<LPARAM>(&search));
        if (result == -1) {
            search.chrg = down ? CHARRANGE{0, text_length} : CHARRANGE{text_length, 0};
            result = SendMessageW(editor_, EM_FINDTEXTEXW, options,
                                  reinterpret_cast<LPARAM>(&search));
        }
        if (result == -1) {
            MessageBeep(MB_ICONINFORMATION);
            return;
        }
        SendMessageW(editor_, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&search.chrgText));
        SendMessageW(editor_, EM_SCROLLCARET, 0, 0);
        SetFocus(editor_);
    }

    void print_document() const {
        PRINTDLGW dialog{sizeof(dialog)};
        dialog.hwndOwner = main_window_;
        dialog.Flags = PD_RETURNDC | PD_NOSELECTION | PD_USEDEVMODECOPIESANDCOLLATE;
        if (!PrintDlgW(&dialog)) {
            return;
        }
        DOCINFOW document{sizeof(document)};
        document.lpszDocName = current_path_.empty() ? L"Document" : current_path_.c_str();
        if (StartDocW(dialog.hDC, &document) <= 0) {
            DeleteDC(dialog.hDC);
            return;
        }

        FORMATRANGE range{};
        range.hdc = dialog.hDC;
        range.hdcTarget = dialog.hDC;
        const int dpi_x = GetDeviceCaps(dialog.hDC, LOGPIXELSX);
        const int dpi_y = GetDeviceCaps(dialog.hDC, LOGPIXELSY);
        range.rcPage = {0, 0,
                        MulDiv(GetDeviceCaps(dialog.hDC, PHYSICALWIDTH), 1440, dpi_x),
                        MulDiv(GetDeviceCaps(dialog.hDC, PHYSICALHEIGHT), 1440, dpi_y)};
        constexpr LONG margin = 1080;
        range.rc = {range.rcPage.left + margin, range.rcPage.top + margin,
                    range.rcPage.right - margin, range.rcPage.bottom - margin};
        range.chrg = {0, -1};

        LONG next = 0;
        const LONG length = GetWindowTextLengthW(editor_);
        bool failed = false;
        while (next < length && !failed) {
            if (StartPage(dialog.hDC) <= 0) {
                failed = true;
                break;
            }
            range.chrg.cpMin = next;
            next = static_cast<LONG>(SendMessageW(editor_, EM_FORMATRANGE, TRUE,
                                                   reinterpret_cast<LPARAM>(&range)));
            if (EndPage(dialog.hDC) <= 0 || next <= range.chrg.cpMin) {
                failed = true;
            }
        }
        SendMessageW(editor_, EM_FORMATRANGE, FALSE, 0);
        failed ? AbortDoc(dialog.hDC) : EndDoc(dialog.hDC);
        DeleteDC(dialog.hDC);
    }

    void show_about() const {
        const std::wstring message =
            L"Microsoft Word for Windows — native x64 migration\n\n"
            L"This executable is AMD64 and contains no 16-bit assembly.\n"
            L"Legacy assembly modules accounted for: " +
            std::to_wstring(opus::x64::assembly_manifest_count()) +
            L"\n\nCurrent document formats: RTF and UTF-8 text.";
        MessageBoxW(main_window_, message.c_str(), L"About", MB_OK | MB_ICONINFORMATION);
    }

    static DWORD CALLBACK stream_in(const DWORD_PTR cookie, const LPBYTE buffer,
                                    const LONG byte_count, LONG* bytes_read) {
        DWORD read = 0;
        if (!ReadFile(reinterpret_cast<HANDLE>(cookie), buffer,
                      static_cast<DWORD>(byte_count), &read, nullptr)) {
            *bytes_read = 0;
            return GetLastError();
        }
        *bytes_read = static_cast<LONG>(read);
        return 0;
    }

    static DWORD CALLBACK stream_out(const DWORD_PTR cookie, const LPBYTE buffer,
                                     const LONG byte_count, LONG* bytes_written) {
        DWORD written = 0;
        if (!WriteFile(reinterpret_cast<HANDLE>(cookie), buffer,
                       static_cast<DWORD>(byte_count), &written, nullptr)) {
            *bytes_written = 0;
            return GetLastError();
        }
        *bytes_written = static_cast<LONG>(written);
        return 0;
    }

    void show_error(const std::wstring_view message, const DWORD error) const {
        const std::wstring detail = std::wstring(message) + L".\n\n" + system_error_message(error);
        MessageBoxW(main_window_, detail.c_str(), kProductName, MB_OK | MB_ICONERROR);
    }

    HINSTANCE instance_{};
    HMODULE rich_edit_library_{};
    HWND main_window_{};
    HWND editor_{};
    HWND status_{};
    HWND find_dialog_{};
    UINT find_message_{};
    FINDREPLACEW find_state_{};
    std::array<wchar_t, 256> find_text_{};
    DWORD find_flags_{FR_DOWN};
    std::wstring current_path_;
    bool loading_{};
    bool status_dirty_{true};
};

}  // namespace

int WINAPI wWinMain(const HINSTANCE instance, HINSTANCE, PWSTR, const int show_command) {
    WordApplication application(instance);
    return application.run(show_command);
}
