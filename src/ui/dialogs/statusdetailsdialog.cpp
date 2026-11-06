#include "statusdetailsdialog.h"
#include "../../utils/theme.h"
#include "../../platform/windoweffects_p.h"

#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QEvent>
#include <QPointer>
#include <QGuiApplication>
#include <QPainter>
#include <QStyleHints>
#if defined(Q_OS_WIN)
#  include <qt_windows.h>
#  include <dwmapi.h>
#endif

StatusDetailsDialog::StatusDetailsDialog(
        const QString &summary, const QString &details, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Connection details"));
#if defined(Q_OS_WIN)
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowSystemMenuHint
            | Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint);
#else
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
#endif
    setWindowModality(Qt::WindowModal);
    resize(520, 320);
#if defined(Q_OS_WIN)
    const auto handle = reinterpret_cast<HWND>(winId());
    const MARGINS margins{ 1, 1, 1, 1 };
    DwmExtendFrameIntoClientArea(handle, &margins);
    const int roundedCorners = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(
            handle, static_cast<DWMWINDOWATTRIBUTE>(33), &roundedCorners, sizeof(roundedCorners));
    SetWindowPos(handle, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
#endif
#if !defined(Q_OS_WIN)
    WindowEffects::prepare(this);
    m_backdropAvailable = WindowEffects::setup(this);
    m_effectsReady = true;
#endif

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    auto *title = new QLabel(summary, this);
    title->setTextFormat(Qt::PlainText);
    title->setWordWrap(true);
    Theme::applyFor(title, Theme::FontRole::BodyBold);
    layout->addWidget(title);
    auto *text = new QPlainTextEdit(this);
    text->setReadOnly(true);
    text->setPlainText(details);
    layout->addWidget(text, 1);
    auto *buttons = new QDialogButtonBox(this);
    auto *copy = buttons->addButton(tr("Copy"), QDialogButtonBox::ActionRole);
    auto *close = buttons->addButton(tr("Close"), QDialogButtonBox::RejectRole);
    copy->setAutoDefault(false);
    close->setAutoDefault(false);
    connect(copy, &QPushButton::clicked, this,
            [details] { QApplication::clipboard()->setText(details); });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    auto *hints = QGuiApplication::styleHints();
    Theme::setTheme(this, hints->colorScheme() == Qt::ColorScheme::Dark);
    connect(hints, &QStyleHints::colorSchemeChanged, this, [this](Qt::ColorScheme scheme) {
        Theme::setTheme(this, scheme == Qt::ColorScheme::Dark);
    });
    if (parent)
        parent->installEventFilter(this);
    anchorToParent();
}

void StatusDetailsDialog::anchorToParent()
{
    if (!parentWidget())
        return;
#if defined(Q_OS_WIN)
    const QSize dialogSize = frameGeometry().size();
#else
    const QSize dialogSize = size();
#endif
    const QPoint target =
            parentWidget()->mapToGlobal(QPoint((parentWidget()->width() - dialogSize.width()) / 2,
                    (parentWidget()->height() - dialogSize.height()) / 2));
    if (pos() != target)
        move(target);
}

bool StatusDetailsDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget()
            && (event->type() == QEvent::Move || event->type() == QEvent::Resize))
        anchorToParent();
    return QDialog::eventFilter(watched, event);
}

bool StatusDetailsDialog::event(QEvent *event)
{
    const QPointer<StatusDetailsDialog> guard(this);
    const bool handled = QDialog::event(event);
    // DeferredDelete (or a signal handler) may destroy the window in the base handler.
    if (!guard)
        return handled;
    if (event->type() == QEvent::Show)
        anchorToParent();
    if (!m_effectsReady)
        return handled;
    switch (event->type()) {
    case QEvent::Show:
    case QEvent::Move:
        anchorToParent();
        break;
    default:
        break;
    }
    switch (event->type()) {
    case QEvent::Show:
    case QEvent::WinIdChange:
    case QEvent::ThemeChange:
    case QEvent::PaletteChange:
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

void StatusDetailsDialog::paintEvent(QPaintEvent *event)
{
    QDialog::paintEvent(event);
#if !defined(Q_OS_WIN)
    QColor background = palette().color(QPalette::Window);
#  if defined(Q_OS_MACOS)
    if (m_backdropAvailable && isActiveWindow())
        background.setAlpha(140);
#  elif defined(Q_OS_WIN)
    if (m_backdropAvailable)
        background.setAlpha(0);
#  endif
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), background);
#endif
}

#if defined(Q_OS_WIN)
bool StatusDetailsDialog::nativeEvent(const QByteArray &type, void *message, qintptr *result)
{
    const auto *msg = static_cast<MSG *>(message);
    if (msg->message == WM_NCCALCSIZE && msg->wParam) {
        *result = 0;
        return true;
    }
    if (msg->message == WM_NCHITTEST) {
        *result = HTCLIENT;
        return true;
    }
    if (msg->message == WM_SYSCOMMAND && (msg->wParam & 0xFFF0) == SC_MOVE) {
        *result = 0;
        return true;
    }
    const bool handled = QDialog::nativeEvent(type, message, result);
    if (m_effectsReady
            && (msg->message == WM_SETTINGCHANGE || msg->message == WM_THEMECHANGED
                    || msg->message == WM_DWMCOMPOSITIONCHANGED
                    || msg->message == WM_DWMCOLORIZATIONCOLORCHANGED)) {
        m_backdropAvailable = WindowEffects::refresh(this);
        update();
    }
    return handled;
}
#endif
