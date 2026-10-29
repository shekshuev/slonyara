#include "appwindow.h"
#include "../platform/windoweffects_p.h"

#include <QEvent>
#include <QPainter>
#include <QPalette>

AppWindow::AppWindow(QWidget *parent) : QMainWindow(parent)
{
    WindowEffects::prepare(this);
    m_backdropAvailable = WindowEffects::setup(this);
    m_effectsReady = true;
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
    if (m_backdropAvailable && isActiveWindow()) {
#if defined(Q_OS_MACOS)
        alpha = 175;
#elif defined(Q_OS_WIN)
        alpha = 0;
#endif
    }
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    QColor background = palette().color(QPalette::Window);
    background.setAlpha(alpha);
    painter.fillRect(rect(), background);
}
