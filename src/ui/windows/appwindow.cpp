#include "appwindow.h"
#include "../../utils/theme.h"
#include "../../platform/windoweffects_p.h"

#include <QEvent>
#include <QGuiApplication>
#include <QStyleHints>
#include <QPainter>
#include <QPalette>

AppWindow::AppWindow(QWidget *parent) : QMainWindow(parent)
{
    WindowEffects::prepare(this);
    m_backdropAvailable = WindowEffects::setup(this);
    m_effectsReady = true;
    auto *hints = QGuiApplication::styleHints();
    Theme::setTheme(this, hints->colorScheme() == Qt::ColorScheme::Dark);
    connect(hints, &QStyleHints::colorSchemeChanged, this, [this](Qt::ColorScheme scheme) {
        Theme::setTheme(this, scheme == Qt::ColorScheme::Dark);
    });
}

bool AppWindow::event(QEvent *event)
{
    const bool handled = QMainWindow::event(event);
    if (!m_effectsReady)
        return handled;

    switch (event->type()) {
    case QEvent::Show:
    case QEvent::WinIdChange:
    case QEvent::ThemeChange:
#if defined(Q_OS_WIN)
    case QEvent::PaletteChange:
#endif
        m_backdropAvailable = WindowEffects::refresh(this);
        update();
        break;
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
        update();
        break;
    default:
        break;
    }
    return handled;
}

void AppWindow::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);
    int alpha = 255;
#if defined(Q_OS_MACOS)
    if (m_backdropAvailable && isActiveWindow()) {
        alpha = 140;
    }
#elif defined(Q_OS_WIN)
    if (m_backdropAvailable) {
        alpha = 0;
    }
#endif
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    QColor background = palette().color(QPalette::Window);
    background.setAlpha(alpha);
    painter.fillRect(rect(), background);
}
