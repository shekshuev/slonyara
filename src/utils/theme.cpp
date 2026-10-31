#include "theme.h"

#include <QLabel>
#include <QLineEdit>
#include <QApplication>
#include <QStyle>
#include <QVariant>

namespace {
void refreshStyle(QWidget *widget)
{
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    if (auto *label = qobject_cast<QLabel *>(widget)) {
        const QString roleName = label->property("colorRole").toString();
        const auto role = roleName == "muted" ? Theme::ColorRole::Muted
                         : roleName == "accent" ? Theme::ColorRole::Accent
                                                : Theme::ColorRole::Primary;
        auto palette = label->palette();
        palette.setColor(QPalette::WindowText, Theme::color(role, label));
        label->setPalette(palette);
    }
    widget->updateGeometry();
    widget->update();
}
} // namespace

namespace Theme {

QColor color(ColorRole role, const QWidget *widget)
{
    bool isDark = (widget ? widget->palette() : QApplication::palette())
                      .color(QPalette::Window).lightness() < 128;
    for (const QWidget *ancestor = widget; ancestor; ancestor = ancestor->parentWidget()) {
        const QString theme = ancestor->property("theme").toString();
        if (!theme.isEmpty()) {
            isDark = theme == "dark";
            break;
        }
    }
    switch (role) {
    case ColorRole::SidebarTint:
        return QColor(0xFF, 0xFF, 0xFF, isDark ? 18 : 64);
    case ColorRole::ItemHover:
        return isDark ? QColor(0x2C, 0x2C, 0x2E) : QColor(0xF2, 0xF2, 0xF7);
    case ColorRole::ItemSelected:
        return isDark ? QColor(0x3A, 0x3A, 0x3C) : QColor(0xE5, 0xE5, 0xEA);
    case ColorRole::ItemBorder:
        return QColor(0x8E, 0x8E, 0x93);
    case ColorRole::Muted:
        return QColor(0x8E, 0x8E, 0x93);
    case ColorRole::Accent:
        return isDark ? QColor(0x0A, 0x84, 0xFF) : QColor(0x00, 0x7A, 0xFF);
    default:
        return isDark ? QColor(0xFF, 0xFF, 0xFF) : QColor(0x1C, 0x1C, 0x1E);
    }
}

QFont font(FontRole role)
{
    switch (role) {
    case FontRole::Title:
        return QFont("Xolonium", 24, QFont::Bold);
    case FontRole::Header:
        return QFont("Manrope", 16, QFont::Bold);
    case FontRole::BodyBold:
        return QFont("Manrope", 14, QFont::Bold);
    case FontRole::Caption:
        return QFont("Manrope", 12);
    default:
        return QFont("Manrope", 11);
    }
}

void applyFor(QLabel *label, FontRole font, ColorRole color)
{
    if (!label)
        return;

    label->setFont(Theme::font(font));

    switch (font) {
    case FontRole::Title:
        label->setProperty("fontRole", "title");
        break;
    case FontRole::Header:
        label->setProperty("fontRole", "header");
        break;
    case FontRole::BodyBold:
        label->setProperty("fontRole", "body-bold");
        break;
    case FontRole::Caption:
        label->setProperty("fontRole", "caption");
        break;
    default:
        label->setProperty("fontRole", "default");
        break;
    }

    switch (color) {
    case ColorRole::Muted:
        label->setProperty("colorRole", "muted");
        break;
    case ColorRole::Accent:
        label->setProperty("colorRole", "accent");
        break;
    default:
        label->setProperty("colorRole", "primary");
        break;
    }

    refreshStyle(label);
}

void applyFor(QLineEdit *input)
{
    if (!input)
        return;

    input->setProperty("controlRole", "input");
#if defined(Q_OS_MACOS)
    input->setProperty("platform", "macos");
#else
    input->setProperty("platform", "windows");
#endif
    refreshStyle(input);
}

void setTheme(QWidget *mainWindow, bool isDark)
{
    if (!mainWindow)
        return;

    const QString theme = isDark ? QStringLiteral("dark") : QStringLiteral("light");
    if (mainWindow->property("theme").toString() == theme)
        return;
    mainWindow->setProperty("theme", theme);

    refreshStyle(mainWindow);
    for (auto *child : mainWindow->findChildren<QWidget *>()) {
        refreshStyle(child);
    }
}
} // namespace Theme
