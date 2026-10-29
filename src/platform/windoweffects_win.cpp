#include "windoweffects_p.h"
#include "../ui/appwindow.h"

#include <QOperatingSystemVersion>
#include <QSettings>
#include <qt_windows.h>
#include <dwmapi.h>

namespace {
constexpr auto darkModeAttribute = static_cast<DWMWINDOWATTRIBUTE>(20);
constexpr auto backdropAttribute = static_cast<DWMWINDOWATTRIBUTE>(38);
constexpr auto legacyMicaAttribute = static_cast<DWMWINDOWATTRIBUTE>(1029);
constexpr int micaBackdrop = 2;
constexpr int noBackdrop = 1;

struct AccentPolicy {
    int state;
    int flags;
    DWORD color;
    int animation;
};
struct CompositionAttributeData {
    int attribute;
    void *data;
    SIZE_T size;
};
using SetCompositionAttribute = BOOL (WINAPI *)(HWND, CompositionAttributeData *);

bool setAcrylic(HWND handle, bool enabled)
{
    static const auto setAttribute = reinterpret_cast<SetCompositionAttribute>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));
    if (!setAttribute)
        return false;

    AccentPolicy policy{enabled ? 4 : 0, 0, 0xD91E1E1E, 0};
    CompositionAttributeData data{19, &policy, sizeof(policy)};
    return setAttribute(handle, &data);
}

bool transparencyAllowed()
{
    HIGHCONTRASTW contrast{};
    contrast.cbSize = sizeof(contrast);
    if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0)
        && (contrast.dwFlags & HCF_HIGHCONTRASTON)) {
        return false;
    }
    const QSettings settings(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
        QSettings::NativeFormat);
    return settings.value(QStringLiteral("EnableTransparency"), 1).toBool();
}
}

namespace WindowEffects {
void prepare(QMainWindow *window)
{
    window->setAttribute(Qt::WA_TranslucentBackground);
}

bool setup(QMainWindow *window)
{
    window->winId();
    return refresh(window);
}

bool refresh(QMainWindow *window)
{
    const auto handle = reinterpret_cast<HWND>(window->internalWinId());
    if (!handle)
        return false;

    const BOOL dark = TRUE;
    DwmSetWindowAttribute(handle, darkModeAttribute, &dark, sizeof(dark));

    const int build = QOperatingSystemVersion::current().microVersion();
    const bool allowed = transparencyAllowed();
    bool enabled = false;
    if (build >= 17134)
        setAcrylic(handle, false);
    if (build >= 22621) {
        const int backdrop = allowed ? micaBackdrop : noBackdrop;
        enabled = SUCCEEDED(DwmSetWindowAttribute(handle, backdropAttribute,
                                                  &backdrop, sizeof(backdrop))) && allowed;
    } else if (build >= 22000 && build < 22523) {
        const BOOL mica = allowed;
        enabled = SUCCEEDED(DwmSetWindowAttribute(handle, legacyMicaAttribute,
                                                  &mica, sizeof(mica))) && allowed;
    }
    if (build >= 17134 && allowed && !enabled)
        enabled = setAcrylic(handle, true);

    const int margin = enabled ? -1 : 0;
    const MARGINS margins{margin, margin, margin, margin};
    if (FAILED(DwmExtendFrameIntoClientArea(handle, &margins)))
        return false;
    return enabled;
}
}

bool AppWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    const bool handled = QMainWindow::nativeEvent(eventType, message, result);
    const auto *msg = static_cast<MSG *>(message);
    if (m_effectsReady && (msg->message == WM_SETTINGCHANGE
                          || msg->message == WM_THEMECHANGED
                          || msg->message == WM_DWMCOMPOSITIONCHANGED)) {
        m_backdropAvailable = WindowEffects::refresh(this);
        update();
    }
    return handled;
}
