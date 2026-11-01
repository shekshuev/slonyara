#include "connectiondialog.h"
#include "../../platform/windoweffects_p.h"
#include "../../utils/theme.h"

#include <QDialogButtonBox>
#include <QEvent>
#include <QFormLayout>
#include <QGuiApplication>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QStyleHints>
#include <QVBoxLayout>
#if defined(Q_OS_WIN)
#include <qt_windows.h>
#endif

ConnectionDialog::ConnectionDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("New Connection"));
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setWindowModality(Qt::WindowModal);
    setFixedSize(parent->width() - 48, parent->height() - 48);
    WindowEffects::prepare(this);
    m_backdropAvailable = WindowEffects::setup(this);
    m_effectsReady = true;

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
    }
    layout->addLayout(form);
    layout->addStretch();
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setText(tr("Save"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    buttons->addButton(tr("Test Connection"), QDialogButtonBox::ActionRole);
    for (auto *button : buttons->buttons()) {
        auto *pushButton = qobject_cast<QPushButton *>(button);
        if (pushButton) {
            pushButton->setAutoDefault(false);
            pushButton->setDefault(false);
        }
    }
    connect(buttons, &QDialogButtonBox::accepted, this, &ConnectionDialog::submit);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
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
    const QPoint target = parentWidget()->mapToGlobal(
        QPoint((parentWidget()->width() - width()) / 2,
               (parentWidget()->height() - height()) / 2));
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
}

#if defined(Q_OS_WIN)
bool ConnectionDialog::nativeEvent(const QByteArray &type, void *message, qintptr *result)
{
    const bool handled = QDialog::nativeEvent(type, message, result);
    const auto *msg = static_cast<MSG *>(message);
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

void ConnectionDialog::submit()
{
    if (!validate())
        return;
    const int port = m_port->text().toInt();
    ConnectionProfile profile;
    profile.name = m_name->text().trimmed();
    profile.host = m_host->text().trimmed();
    profile.port = static_cast<quint16>(port);
    profile.database = m_database->text().trimmed();
    profile.user = m_user->text().trimmed();
    emit connectionSubmitted(profile);
}
