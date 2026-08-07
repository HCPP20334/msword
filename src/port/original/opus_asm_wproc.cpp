#define WIN32_LEAN_AND_MEAN
#define NODDE
#include <windows.h>

#include <cstddef>

/*
 * AMD64 translation of Opus/asm/wprocn.asm.  The assembly was not a simple
 * forwarding thunk: it admitted only a table of messages to each original C
 * procedure and sent every other message directly to DefWindowProc.  That
 * distinction is required on Win64, notably for WM_NCCREATE.
 */
extern "C" {

LRESULT CALLBACK AppWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MwdWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WwPaneWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DdeChnlWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RSBWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK FedtWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK StaticEditWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK StartWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DeskTopWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PgPrvwWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SplitBarWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK StatLineWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK IconBarWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RulerMarkWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PromptWndProc(HWND, UINT, WPARAM, LPARAM);

extern HCURSOR vhcIBeam;
extern HCURSOR vhcArrow;
int OurSetCursor(HCURSOR cursor);
void OpusDrawWin95HorizontalRuler(HWND ruler);

}  // extern "C"

namespace {

constexpr UINT kWmSystemError = 0x0017;
constexpr UINT kWmSetVisible = 0x0009;
constexpr UINT kWmCbtInit = 0x03f0;
constexpr UINT kWmCbtTerm = 0x03f1;
constexpr UINT kWmCbtSemEv = 0x03f3;
constexpr UINT kWmDdeInitiate = 0x03e0;
constexpr UINT kWmDdeTerminate = 0x03e1;
constexpr UINT kWmDdeAdvise = 0x03e2;
constexpr UINT kWmDdeUnadvise = 0x03e3;
constexpr UINT kWmDdeAck = 0x03e4;
constexpr UINT kWmDdeData = 0x03e5;
constexpr UINT kWmDdeRequest = 0x03e6;
constexpr UINT kWmDdePoke = 0x03e7;
constexpr UINT kWmDdeExecute = 0x03e8;
constexpr UINT kWmOpusX64QuerySelection = WM_APP + 0x351;

enum class MouseCursor { none, ibeam, arrow };

template <std::size_t Count>
bool Contains(const UINT (&messages)[Count], const UINT message) {
    for (const UINT candidate : messages) {
        if (candidate == message) {
            return true;
        }
    }
    return false;
}

template <std::size_t Count>
LRESULT Dispatch(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam,
                 WNDPROC source, const UINT (&messages)[Count],
                 MouseCursor mouse_cursor = MouseCursor::none) {
    if (Contains(messages, message)) {
        return source(hwnd, message, wParam, lParam);
    }
    if (message == WM_MOUSEMOVE && mouse_cursor != MouseCursor::none) {
        OurSetCursor(mouse_cursor == MouseCursor::ibeam ? vhcIBeam : vhcArrow);
        return FALSE;
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

constexpr UINT kAppMessages[] = {
    WM_CREATE, WM_INITMENUPOPUP, WM_MENUCHAR, WM_ENTERIDLE, WM_ACTIVATE,
    WM_ACTIVATEAPP, WM_MOUSEACTIVATE, WM_SETCURSOR, WM_TIMER, WM_CLOSE,
    WM_QUERYENDSESSION, WM_ENDSESSION, WM_DESTROY, WM_SIZE, WM_MOVE,
    WM_COMMAND, WM_SYSCOMMAND, WM_SYSCOLORCHANGE, WM_WININICHANGE,
    WM_DEVMODECHANGE, WM_DESTROYCLIPBOARD, WM_RENDERFORMAT,
    WM_PAINTCLIPBOARD, WM_VSCROLLCLIPBOARD, WM_HSCROLLCLIPBOARD,
    WM_SIZECLIPBOARD, WM_ASKCBFORMATNAME, WM_FONTCHANGE, kWmDdeInitiate,
    WM_MENUSELECT, WM_SYSKEYDOWN, kWmCbtInit, kWmCbtTerm, kWmCbtSemEv,
    kWmSystemError, WM_NCLBUTTONDBLCLK};
constexpr UINT kMwdMessages[] = {
    WM_CREATE, WM_MOVE, WM_SIZE, WM_CLOSE, WM_GETMINMAXINFO, WM_VSCROLL,
    WM_HSCROLL, WM_MOUSEACTIVATE, WM_CHILDACTIVATE, WM_MENUCHAR,
    WM_SYSCOMMAND, WM_INITMENUPOPUP, WM_MENUSELECT, WM_ENTERIDLE};
constexpr UINT kWwPaneMessages[] = {
    WM_CREATE, WM_SIZE, WM_SETFOCUS, WM_KILLFOCUS, WM_PAINT,
    WM_MOUSEACTIVATE, WM_SETCURSOR, WM_MOUSEMOVE, WM_LBUTTONDOWN,
    WM_LBUTTONDBLCLK, WM_RBUTTONDOWN, WM_SYSCOMMAND, WM_MENUCHAR,
    kWmOpusX64QuerySelection};
constexpr UINT kDdeMessages[] = {
    kWmDdeTerminate, kWmDdeAdvise, kWmDdeUnadvise, kWmDdeAck,
    kWmDdeData, kWmDdeRequest, kWmDdePoke, kWmDdeExecute};
constexpr UINT kRsbMessages[] = {WM_NCCREATE, WM_CREATE, WM_ERASEBKGND,
                                  WM_PAINT, WM_LBUTTONDOWN,
                                  WM_LBUTTONDBLCLK, WM_SETCURSOR};
constexpr UINT kFedtMessages[] = {
    WM_NCCREATE, WM_DESTROY, WM_SIZE, WM_SETFOCUS, WM_KILLFOCUS, WM_KEYDOWN,
    WM_CHAR, WM_KEYUP, WM_LBUTTONDBLCLK, WM_LBUTTONDOWN, WM_SETREDRAW,
    WM_ERASEBKGND, WM_PAINT, WM_GETTEXTLENGTH, WM_GETTEXT, WM_SETTEXT,
    EM_SETHANDLE, WM_GETDLGCODE, WM_COPY, WM_CUT, WM_CLEAR, WM_PASTE,
    EM_SETSEL, EM_GETSEL, EM_GETLINECOUNT, EM_REPLACESEL, EM_GETHANDLE};
constexpr UINT kStaticEditMessages[] = {
    WM_CREATE, WM_DESTROY, WM_PAINT, WM_SETFOCUS, WM_KILLFOCUS, WM_COMMAND,
    WM_GETDLGCODE, WM_SETCURSOR, WM_KEYDOWN, WM_MOUSEMOVE, WM_LBUTTONDOWN,
    WM_LBUTTONUP, WM_ENABLE};
constexpr UINT kStartMessages[] = {WM_NCPAINT, WM_DESTROY};
constexpr UINT kDesktopMessages[] = {WM_SIZE};
constexpr UINT kPreviewMessages[] = {
    WM_MOUSEMOVE, WM_CREATE, WM_SIZE, WM_ERASEBKGND, WM_PAINT, WM_VSCROLL,
    WM_LBUTTONDBLCLK, WM_LBUTTONDOWN, WM_CLOSE, WM_SYSCOMMAND};
constexpr UINT kSplitBarMessages[] = {WM_PAINT};
constexpr UINT kStatLineMessages[] = {WM_CREATE, WM_LBUTTONDBLCLK, WM_PAINT,
                                       WM_LBUTTONDOWN};
constexpr UINT kIconBarMessages[] = {
    WM_NCDESTROY, WM_LBUTTONDOWN, WM_LBUTTONDBLCLK, WM_SYSCOMMAND,
    WM_SYSKEYDOWN, WM_KEYDOWN, WM_PAINT, kWmSetVisible, WM_MOVE, WM_SIZE,
    WM_ENABLE};
constexpr UINT kRulerMarkMessages[] = {WM_LBUTTONDBLCLK, WM_CREATE, WM_PAINT,
                                        WM_LBUTTONDOWN};
constexpr UINT kPromptMessages[] = {
    WM_SYSCOMMAND, WM_CREATE, WM_PAINT, WM_DESTROY, WM_TIMER, WM_KEYDOWN,
    WM_CHAR, WM_SETFOCUS, WM_SIZE, WM_KILLFOCUS};

}  // namespace

extern "C" {

LRESULT CALLBACK NatAppWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, AppWndProc, kAppMessages);
}
LRESULT CALLBACK NatMwdWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, MwdWndProc, kMwdMessages);
}
LRESULT CALLBACK NatWwPaneWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, WwPaneWndProc, kWwPaneMessages);
}
LRESULT CALLBACK NatDdeChnlWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, DdeChnlWndProc, kDdeMessages);
}
LRESULT CALLBACK NatRSBWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, RSBWndProc, kRsbMessages);
}
LRESULT CALLBACK NatFedtWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, FedtWndProc, kFedtMessages,
                    MouseCursor::ibeam);
}
LRESULT CALLBACK NatStaticEditWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, StaticEditWndProc, kStaticEditMessages,
                    MouseCursor::ibeam);
}
LRESULT CALLBACK NatStartWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, StartWndProc, kStartMessages,
                    MouseCursor::arrow);
}
LRESULT CALLBACK NatDeskTopWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, DeskTopWndProc, kDesktopMessages,
                    MouseCursor::arrow);
}
LRESULT CALLBACK NatPgPrvwWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, PgPrvwWndProc, kPreviewMessages,
                    MouseCursor::arrow);
}
LRESULT CALLBACK NatSplitBarWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, SplitBarWndProc, kSplitBarMessages,
                    MouseCursor::arrow);
}
LRESULT CALLBACK NatStatLineWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, StatLineWndProc, kStatLineMessages,
                    MouseCursor::arrow);
}
LRESULT CALLBACK NatIconBarWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, IconBarWndProc, kIconBarMessages,
                    MouseCursor::arrow);
}
LRESULT CALLBACK NatRulerMarkWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    const LRESULT result = Dispatch(h, m, w, l, RulerMarkWndProc,
                                    kRulerMarkMessages,
                                    MouseCursor::arrow);
    if (m == WM_PAINT) {
        OpusDrawWin95HorizontalRuler(h);
    }
    return result;
}
LRESULT CALLBACK NatPromptWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, PromptWndProc, kPromptMessages,
                    MouseCursor::arrow);
}

}  // extern "C"
