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
// Mirrors AcrylicCompositor::WINDOWCOMPOSITIONATTRIBDATA

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

// Accent states (matching AcrylicCompositor's usage)
static const DWORD ACCENT_DISABLED                  = 0;
static const DWORD ACCENT_ENABLE_BLURBEHIND         = 3;
static const DWORD ACCENT_ENABLE_ACRYLICBLURBEHIND  = 4;
static const DWORD ACCENT_ENABLE_HOSTBACKDROP       = 5;  // Win11

// WCA attribute IDs
static const DWORD WCA_ACCENT_POLICY       = 19;  // 0x13
static const DWORD WCA_USEDARKMODECOLORS   = 26;

// DWM attributes for Windows 11
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

typedef BOOL (WINAPI *pfnSetWindowCompositionAttribute)(HWND, WCA_DATA*);
typedef LONG (WINAPI *pfnRtlGetVersion)(MY_RTL_OSVERSIONINFOW*);

// Convert QColor to AABBGGRR DWORD (Windows gradient color format)
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

    // Cache the HWND once here. NEVER call winId() again (it can trigger
    // recursive window recreation if called from nativeEventFilter).
    m_hwnd = m_widget->winId();
    HWND hwnd = reinterpret_cast<HWND>(m_hwnd);
    if (!hwnd) return false;

    enableDarkMode();
    // NOTE: Do NOT call extendFrame() or updateClipRegion() here.
    // WA_TranslucentBackground creates a WS_EX_LAYERED window where:
    //  - DwmExtendFrameIntoClientArea is unnecessary and can interfere
    //  - SetWindowRgn is ignored (shape comes from per-pixel alpha)

    long build = getBuildVersion();

    // Windows 11 22H2+ (Build 22621): native acrylic via DwmSetWindowAttribute
    if (build >= 22621 && source == HostBackdrop) {
        m_effectApplied = applyWin11Backdrop();
        if (m_effectApplied) return true;
        // Fall through to legacy path
    }

    // Windows 10 1803+ / Windows 11 older: SetWindowCompositionAttribute
    m_effectApplied = applyAcrylicAccent();
    return m_effectApplied;
#else
    Q_UNUSED(source);
    Q_UNUSED(params);
    return false;
#endif
}

void AcrylicHelper::updateClipRegion()
{
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(m_hwnd);
    if (!hwnd) return;

    int w = m_widget->width();
    int h = m_widget->height();
    int radius = h;  // Pill shape: radius = half height

    HRGN rgn = CreateRoundRectRgn(0, 0, w + 1, h + 1, radius, radius);
    SetWindowRgn(hwnd, rgn, TRUE);
    // Windows takes ownership of the region handle after SetWindowRgn
#endif
}

#ifdef Q_OS_WIN

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
    accent.AccentFlags = 2;  // Transparent gradient
    accent.GradientColor = colorToABGR(m_params.tintColor);

    WCA_DATA data = {};
    data.Attribute = WCA_ACCENT_POLICY;
    data.Data = &accent;
    data.SizeOfData = sizeof(accent);

    return setWCA(hwnd, &data) != FALSE;
}

bool AcrylicHelper::applyWin11Backdrop()
{
    HWND hwnd = reinterpret_cast<HWND>(m_hwnd);
    if (!hwnd) return false;

    int backdropType = 3;  // Acrylic (DWM_SYSTEMBACKDROP_TYPE_TRANSIENT_WINDOW)
    HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE,
                                        &backdropType, sizeof(backdropType));
    return SUCCEEDED(hr);
}

void AcrylicHelper::enableDarkMode()
{
    HWND hwnd = reinterpret_cast<HWND>(m_hwnd);
    if (!hwnd) return;

    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
}

void AcrylicHelper::extendFrame()
{
    HWND hwnd = reinterpret_cast<HWND>(m_hwnd);
    if (!hwnd) return;

    MARGINS margins = {-1, -1, -1, -1};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
}

void AcrylicHelper::syncOnActivate(bool active)
{
    m_active = active;
    // Re-apply accent on activation change to maintain effect
    if (m_effectApplied) {
        applyAcrylicAccent();
    }
}

#endif // Q_OS_WIN

bool AcrylicHelper::nativeEventFilter(const QByteArray& eventType, void* message, long* result)
{
#ifdef Q_OS_WIN
    // IMPORTANT: Use cached m_hwnd, never call m_widget->winId() here.
    // Calling winId() during event processing causes recursive window creation.
    if (m_hwnd && m_effectApplied && eventType == "windows_generic_MSG") {
        MSG* msg = static_cast<MSG*>(message);

        if (msg->hwnd == reinterpret_cast<HWND>(m_hwnd)) {
            switch (msg->message) {
            case WM_ACTIVATE: {
                WORD state = LOWORD(msg->wParam);
                bool active = (state == WA_ACTIVE || state == WA_CLICKACTIVE);
                syncOnActivate(active);
                break;
            }
            }
        }
    }
#else
    Q_UNUSED(eventType);
    Q_UNUSED(message);
#endif
    Q_UNUSED(result);
    return false;  // Never consume the event
}
