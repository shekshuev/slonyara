#include "connectiondialog.h"
#include <QDialogButtonBox>
#include <QEvent>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QGuiApplication>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include "../widgets/statuscard.h"
#include <QPainter>
#include <QPushButton>
#include <QStyleHints>
#include <QVBoxLayout>
#include "../../core/connections/connectiontester.h"
#include "../../platform/windoweffects_p.h"
#include "../../utils/theme.h"
#if defined(Q_OS_WIN)
#include <qt_windows.h>
#include <dwmapi.h>
#endif

ConnectionDialog::ConnectionDialog(QWidget *parent, const ConnectionProfile *profile) : QDialog(parent)
{
    setWindowTitle(profile ? tr("Edit Connection") : tr("New Connection"));
#if defined(Q_OS_WIN)
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowSystemMenuHint
                   | Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint);
#else
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
#endif
    setWindowModality(Qt::WindowModal);
    setFixedSize(parent->width() - 48, parent->height() - 48);
#if defined(Q_OS_WIN)
    const auto handle = reinterpret_cast<HWND>(winId());
    const MARGINS margins{1, 1, 1, 1};
    DwmExtendFrameIntoClientArea(handle, &margins);
    const int roundedCorners = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(handle, static_cast<DWMWINDOWATTRIBUTE>(33),
                         &roundedCorners, sizeof(roundedCorners));
    SetWindowPos(handle, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
#endif
#if !defined(Q_OS_WIN)
    WindowEffects::prepare(this);
    m_backdropAvailable = WindowEffects::setup(this);
    m_effectsReady = true;
#endif

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(12);
    auto *title = new QLabel(windowTitle(), this);
    Theme::applyFor(title, Theme::FontRole::Header);
    layout->addWidget(title);
    auto *form = new QFormLayout;
    form->setVerticalSpacing(10);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_name = new QLineEdit(this);
    m_host = new QLineEdit(QStringLiteral("localhost"), this);
    m_port = new QLineEdit(QStringLiteral("5432"), this);
    m_port->setValidator(new QIntValidator(1, 65535, m_port));
    m_database = new QLineEdit(QStringLiteral("postgres"), this);
    m_user = new QLineEdit(QStringLiteral("postgres"), this);
    m_password = new QLineEdit(this);
    m_password->setEchoMode(QLineEdit::Password);
    if (profile) {
        m_connectionId = profile->id;
        m_name->setText(profile->name);
        m_host->setText(profile->host);
        m_port->setText(QString::number(profile->port));
        m_database->setText(profile->database);
        m_user->setText(profile->user);
    }
    const QList<QPair<QString, QLineEdit *>> fields = {
        {tr("Name"), m_name}, {tr("Host"), m_host}, {tr("Port"), m_port},
        {tr("Database"), m_database}, {tr("User"), m_user},
        {tr("Password"), m_password}
    };
    for (const auto &field : fields) {
        Theme::applyFor(field.second);
        field.second->setClearButtonEnabled(true);
        auto *label = new QLabel(field.first, this);
        Theme::applyFor(label);
        label->setBuddy(field.second);
        form->addRow(label, field.second);
        connect(field.second, &QLineEdit::returnPressed, this, &ConnectionDialog::submit);
        connect(field.second, &QLineEdit::textChanged, this, [this] {
            ++m_formRevision;
            if (m_statusCard)
                m_statusCard->clear();
        });
    }
    layout->addLayout(form);
    layout->addStretch();
    m_statusCard = new StatusCard(this);
    layout->addWidget(m_statusCard);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setText(tr("Save"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    m_testButton = buttons->addButton(tr("Test Connection"), QDialogButtonBox::ActionRole);
    for (auto *button : buttons->buttons()) {
        auto *pushButton = qobject_cast<QPushButton *>(button);
        if (pushButton) {
            pushButton->setAutoDefault(false);
            pushButton->setDefault(false);
        }
    }
    connect(buttons, &QDialogButtonBox::accepted, this, &ConnectionDialog::submit);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_testButton, &QPushButton::clicked, this, &ConnectionDialog::testConnection);
    layout->addWidget(buttons);

    auto *hints = QGuiApplication::styleHints();
    Theme::setTheme(this, hints->colorScheme() == Qt::ColorScheme::Dark);
    connect(hints, &QStyleHints::colorSchemeChanged, this, [this](Qt::ColorScheme scheme) {
        Theme::setTheme(this, scheme == Qt::ColorScheme::Dark);
    });
    parent->installEventFilter(this);
    anchorToParent();
    m_name->setFocus();
}

void ConnectionDialog::anchorToParent()
{
#if defined(Q_OS_WIN)
    const QSize dialogSize = frameGeometry().size();
#else
    const QSize dialogSize = size();
#endif
    const QPoint target = parentWidget()->mapToGlobal(
        QPoint((parentWidget()->width() - dialogSize.width()) / 2,
               (parentWidget()->height() - dialogSize.height()) / 2));
    if (pos() != target)
        move(target);
}

bool ConnectionDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget()
        && (event->type() == QEvent::Move || event->type() == QEvent::Resize))
        anchorToParent();
    return QDialog::eventFilter(watched, event);
}

bool ConnectionDialog::event(QEvent *event)
{
    const bool handled = QDialog::event(event);
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

void ConnectionDialog::paintEvent(QPaintEvent *event)
{
    QDialog::paintEvent(event);
#if !defined(Q_OS_WIN)
    QColor background = palette().color(QPalette::Window);
#if defined(Q_OS_MACOS)
    if (m_backdropAvailable && isActiveWindow())
        background.setAlpha(140);
#elif defined(Q_OS_WIN)
    if (m_backdropAvailable)
        background.setAlpha(0);
#endif
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), background);
#endif
}

#if defined(Q_OS_WIN)
bool ConnectionDialog::nativeEvent(const QByteArray &type, void *message, qintptr *result)
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
    if (m_effectsReady && (msg->message == WM_SETTINGCHANGE
                          || msg->message == WM_THEMECHANGED
                          || msg->message == WM_DWMCOMPOSITIONCHANGED
                          || msg->message == WM_DWMCOLORIZATIONCOLORCHANGED)) {
        m_backdropAvailable = WindowEffects::refresh(this);
        update();
    }
    return handled;
}
#endif

bool ConnectionDialog::validate()
{
    for (auto *field : {m_name, m_host, m_database, m_user}) {
        if (field->text().trimmed().isEmpty()) {
            field->setFocus();
            return false;
        }
    }
    bool ok = false;
    const int port = m_port->text().toInt(&ok);
    if (!ok || port < 1 || port > 65535) {
        m_port->setFocus();
        return false;
    }
    return true;
}

ConnectionProfile ConnectionDialog::toProfile()
{
    const int port = m_port->text().toInt();
    ConnectionProfile profile;
    profile.id = m_connectionId;
    profile.name = m_name->text().trimmed();
    profile.host = m_host->text().trimmed();
    profile.port = static_cast<quint16>(port);
    profile.database = m_database->text().trimmed();
    profile.user = m_user->text().trimmed();
    return profile;
}

void ConnectionDialog::submit()
{
    if (!validate())
        return;
    auto profile = toProfile();
    emit connectionSubmitted(profile);
}

void ConnectionDialog::testConnection()
{
    if (!m_testButton->isEnabled() || !validate())
        return;

    const auto profile = toProfile();
    const auto connectionPassword = m_password->text();
    const auto revision = m_formRevision;
    m_statusCard->setStatus(tr("Testing connection…"), Theme::ColorRole::Info);
    m_testButton->setEnabled(false);
    m_testButton->setText(tr("Testing…"));

    auto *watcher = new QFutureWatcher<ConnectionTestResult>(this);
    connect(watcher, &QFutureWatcher<ConnectionTestResult>::finished, this, [this, watcher, revision] {
        const auto result = watcher->result();
        watcher->deleteLater();
        m_testButton->setEnabled(true);
        m_testButton->setText(tr("Test Connection"));
        if (revision != m_formRevision)
            return;
        if (result.success)
            m_statusCard->setStatus(tr("Connection successful."), Theme::ColorRole::Success);
        else
            m_statusCard->setStatus(tr("Could not connect."), Theme::ColorRole::Error, result.error);
    });
    watcher->setFuture(QtConcurrent::run([profile, connectionPassword] {
        return ConnectionTester::test(profile, connectionPassword);
    }));
}
