#include "AcrylicHelper.h"

#include <QWidget>
#include <QApplication>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")

// RTL_OSVERSIONINFOW for RtlGetVersion (avoid pulling in winternl.h)
typedef struct _MY_RTL_OSVERSIONINFOW {
    ULONG dwOSVersionInfoSize;
    ULONG dwMajorVersion;
    ULONG dwMinorVersion;
    ULONG dwBuildNumber;
    ULONG dwPlatformId;
    WCHAR szCSDVersion[128];
} MY_RTL_OSVERSIONINFOW;

// --- Undocumented SetWindowCompositionAttribute structures ---
struct ACCENT_POLICY {
    DWORD AccentState;
    DWORD AccentFlags;
    DWORD GradientColor;   // AABBGGRR
    DWORD AnimationId;
};

struct WCA_DATA {
    DWORD Attribute;
    PVOID Data;
    SIZE_T SizeOfData;
};

static const DWORD ACCENT_ENABLE_ACRYLICBLURBEHIND  = 4;
static const DWORD WCA_ACCENT_POLICY = 19;

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

typedef BOOL (WINAPI *pfnSetWindowCompositionAttribute)(HWND, WCA_DATA*);
typedef LONG (WINAPI *pfnRtlGetVersion)(MY_RTL_OSVERSIONINFOW*);

static DWORD colorToABGR(const QColor& c)
{
    return (static_cast<DWORD>(c.alpha()) << 24) |
           (static_cast<DWORD>(c.blue())  << 16) |
           (static_cast<DWORD>(c.green()) << 8)  |
           (static_cast<DWORD>(c.red()));
}

#endif // Q_OS_WIN


AcrylicHelper::AcrylicHelper(QWidget* targetWidget, QObject* parent)
    : QObject(parent), m_widget(targetWidget)
{
    qApp->installNativeEventFilter(this);
}

AcrylicHelper::~AcrylicHelper()
{
    qApp->removeNativeEventFilter(this);
}

long AcrylicHelper::getBuildVersion()
{
#ifdef Q_OS_WIN
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll) {
        auto fn = reinterpret_cast<pfnRtlGetVersion>(GetProcAddress(ntdll, "RtlGetVersion"));
        if (fn) {
            MY_RTL_OSVERSIONINFOW info = {};
            info.dwOSVersionInfoSize = sizeof(info);
            if (fn(&info) == 0)
                return static_cast<long>(info.dwBuildNumber);
        }
    }
#endif
    return 0;
}

bool AcrylicHelper::setAcrylicEffect(BackdropSource source, const EffectParams& params)
{
#ifdef Q_OS_WIN
    m_params = params;
    m_source = source;

    // Cache the HWND once here. NEVER call winId() again.
    m_hwnd = m_widget->winId();
    HWND hwnd = reinterpret_cast<HWND>(m_hwnd);
    if (!hwnd) return false;

    // Add WS_THICKFRAME so DWM has a frame to extend.
    // Qt's FramelessWindowHint uses WS_POPUP which has no DWM frame,
    // making DwmExtendFrameIntoClientArea a no-op.
    // We handle WM_NCCALCSIZE to hide the thick frame visually.
    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    SetWindowLongW(hwnd, GWL_STYLE, style | WS_THICKFRAME);

    // Apply pill-shaped window region to clip the entire window (including acrylic).
    // SetWindowRgn works on non-layered windows (no WA_TranslucentBackground).
    applyWindowRegion();

    enableDarkMode();

    // Extend DWM frame into the entire client area.
    // Black pixels become transparent to the frame, revealing the acrylic.
    MARGINS margins = {-1, -1, -1, -1};
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    // Apply acrylic via accent policy.
    m_effectApplied = applyAcrylicAccent();
    if (!m_effectApplied) return false;

    return true;
#else
    Q_UNUSED(source);
    Q_UNUSED(params);
    return false;
#endif
}

void AcrylicHelper::updateBlurRegion()
{
#ifdef Q_OS_WIN
    if (!m_hwnd || !m_effectApplied) return;
    applyWindowRegion();
#endif
}

#ifdef Q_OS_WIN

void AcrylicHelper::applyWindowRegion()
{
    HWND hwnd = reinterpret_cast<HWND>(m_hwnd);
    if (!hwnd) return;

    int w = m_widget->width();
    int h = m_widget->height();

    // Pill-shaped window region
    HRGN rgn = CreateRoundRectRgn(0, 0, w + 1, h + 1, h, h);
    SetWindowRgn(hwnd, rgn, TRUE);  // OS takes ownership of rgn, do NOT DeleteObject
}

bool AcrylicHelper::applyAcrylicAccent()
{
    HWND hwnd = reinterpret_cast<HWND>(m_hwnd);
    if (!hwnd) return false;

    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (!user32) return false;

    auto setWCA = reinterpret_cast<pfnSetWindowCompositionAttribute>(
        GetProcAddress(user32, "SetWindowCompositionAttribute"));
    if (!setWCA) return false;

    ACCENT_POLICY accent = {};
    accent.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
    accent.AccentFlags = 0;
    accent.GradientColor = colorToABGR(m_params.tintColor);

    WCA_DATA data = {};
    data.Attribute = WCA_ACCENT_POLICY;
    data.Data = &accent;
    data.SizeOfData = sizeof(accent);

    return setWCA(hwnd, &data) != FALSE;
}

void AcrylicHelper::enableDarkMode()
{
    HWND hwnd = reinterpret_cast<HWND>(m_hwnd);
    if (!hwnd) return;

    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
}

void AcrylicHelper::syncOnActivate(bool active)
{
    m_active = active;
    if (m_effectApplied) {
        applyAcrylicAccent();
    }
}

#endif // Q_OS_WIN

bool AcrylicHelper::nativeEventFilter(const QByteArray& eventType, void* message, long* result)
{
#ifdef Q_OS_WIN
    if (eventType == "windows_generic_MSG") {
        MSG* msg = static_cast<MSG*>(message);

        if (m_hwnd && msg->hwnd == reinterpret_cast<HWND>(m_hwnd)) {
            switch (msg->message) {
            case WM_NCCALCSIZE:
                // Make client area = entire window (hides the WS_THICKFRAME border).
                if (msg->wParam == TRUE) {
                    *result = 0;
                    return true;
                }
                break;

            case WM_ERASEBKGND:
                // Prevent the system from erasing the background.
                *result = 1;
                return true;

            case WM_NCHITTEST: {
                // Prevent resize cursors from the thick frame.
                // Return HTCLIENT for all areas.
                *result = HTCLIENT;
                return true;
            }

            case WM_ACTIVATE:
                if (m_effectApplied) {
                    WORD state = LOWORD(msg->wParam);
                    bool active = (state == WA_ACTIVE || state == WA_CLICKACTIVE);
                    syncOnActivate(active);
                }
                break;

            case WM_SIZE:
                if (m_effectApplied) {
                    updateBlurRegion();
                }
                break;
            }
        }
    }
#else
    Q_UNUSED(eventType);
    Q_UNUSED(message);
#endif
    Q_UNUSED(result);
    return false;
}
