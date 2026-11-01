#include "../ui/windows/appwindow.h"
#include "windoweffects_p.h"

#include <QOperatingSystemVersion>
#include <QSettings>
#include <QPalette>
#include <QGuiApplication>
#include <QStyleHints>
#include <qt_windows.h>
#include <dwmapi.h>
#include <uxtheme.h>

namespace {
constexpr auto darkModeAttribute = static_cast<DWMWINDOWATTRIBUTE>(20);
constexpr auto backdropAttribute = static_cast<DWMWINDOWATTRIBUTE>(38);
constexpr auto captionColorAttribute = static_cast<DWMWINDOWATTRIBUTE>(35);
constexpr COLORREF defaultDwmColor = 0xFFFFFFFF;
constexpr int micaBackdrop = 2;
constexpr int noBackdrop = 1;

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
void prepare(QWidget *window)
{
    window->setAttribute(Qt::WA_TranslucentBackground);
}

bool setup(QWidget *window)
{
    window->winId();
    return refresh(window);
}

bool refresh(QWidget *window)
{
    const auto handle = reinterpret_cast<HWND>(window->internalWinId());
    if (!handle)
        return false;

    constexpr DWORD hiddenCaptionParts = WTNCA_NODRAWCAPTION | WTNCA_NODRAWICON;
    WTA_OPTIONS captionOptions{hiddenCaptionParts, hiddenCaptionParts};
    SetWindowThemeAttribute(handle, WTA_NONCLIENT, &captionOptions, sizeof(captionOptions));

    const BOOL dark = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    DwmSetWindowAttribute(handle, darkModeAttribute, &dark, sizeof(dark));

    const int build = QOperatingSystemVersion::current().microVersion();
    const bool allowed = transparencyAllowed();
    bool enabled = false;
    if (build >= 22621) {
        const int backdrop = allowed ? micaBackdrop : noBackdrop;
        enabled = SUCCEEDED(DwmSetWindowAttribute(handle, backdropAttribute,
                                                  &backdrop, sizeof(backdrop))) && allowed;
    }

    const QColor background = window->palette().color(QPalette::Window);
    const COLORREF captionColor = enabled ? defaultDwmColor
        : RGB(background.red(), background.green(), background.blue());
    DwmSetWindowAttribute(handle, captionColorAttribute, &captionColor, sizeof(captionColor));

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
                          || msg->message == WM_DWMCOLORIZATIONCOLORCHANGED
                          || msg->message == WM_DWMCOMPOSITIONCHANGED)) {
        m_backdropAvailable = WindowEffects::refresh(this);
        update();
    }
    return handled;
}
