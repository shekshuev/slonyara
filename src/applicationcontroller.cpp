#include "applicationcontroller.h"

#include "ui/windows/introwindow.h"
#include "ui/windows/mainwindow.h"
#include "core/connections/postgressession.h"
#include "core/connections/connectionsecrets.h"
#include "utils/theme.h"
#include <QInputDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QTimer>
#include <algorithm>

ApplicationController::ApplicationController(QObject *parent)
    : ApplicationController(Dependencies{ ConnectionSecrets::read,
                                    [](const ConnectionProfile &profile, const QString &password) {
                                        return std::make_unique<PostgresSession>(profile, password);
                                    } },
              parent)
{
}

ApplicationController::ApplicationController(Dependencies dependencies, QObject *parent)
    : QObject(parent),
      m_dependencies(std::move(dependencies)),
      m_introWindow(std::make_unique<IntroWindow>())
{
    connect(m_introWindow.get(), &IntroWindow::connectionRequested, this,
            &ApplicationController::openWorkspace);
    connect(m_introWindow.get(), &IntroWindow::connectionCancelled, this,
            &ApplicationController::cancelConnection);
}

ApplicationController::~ApplicationController()
{
    m_mainWindow.reset();
    m_session.reset();
}

void ApplicationController::start()
{
    m_introWindow->show();
}

void ApplicationController::openWorkspace(const ConnectionProfile &profile)
{
    if (m_mainWindow || m_connecting || m_session)
        return;
    m_connecting = true;
    m_connectionError.clear();
    m_introWindow->setConnecting(true);
    m_connectingTimer.start();
    const auto attempt = ++m_attempt;
    if (!profile.savePassword) {
        requestPassword(profile);
        return;
    }
    m_dependencies.readPassword(profile.id, this, [this, profile, attempt](const auto &result) {
        if (attempt != m_attempt)
            return;
        if (!result.success) {
            completeConnection(attempt, false, result.error);
        } else if (result.found) {
            connectProfile(profile, result.password);
        } else {
            requestPassword(profile);
        }
    });
}

void ApplicationController::requestPassword(const ConnectionProfile &profile)
{
    auto *dialog = new QInputDialog(m_introWindow.get());
    m_passwordDialog = dialog;
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("Connect to %1").arg(profile.name));
    dialog->setLabelText(tr("Password for %1:").arg(profile.user));
    dialog->setTextEchoMode(QLineEdit::Password);
#if defined(Q_OS_MACOS)
    Theme::applyFor(dialog->findChild<QLineEdit *>());
#endif
    const auto attempt = m_attempt;
    connect(dialog, &QDialog::accepted, this, [this, profile, dialog, attempt] {
        m_passwordDialog = nullptr;
        if (attempt == m_attempt)
            connectProfile(profile, dialog->textValue());
        dialog->setTextValue({});
    });
    connect(dialog, &QDialog::rejected, this, [this, attempt] {
        m_passwordDialog = nullptr;
        if (attempt == m_attempt)
            cancelConnection();
    });
    dialog->open();
#if defined(Q_OS_MACOS)
    if (auto *buttons = dialog->findChild<QDialogButtonBox *>())
        buttons->setCenterButtons(true);
#endif
}

void ApplicationController::connectProfile(
        const ConnectionProfile &profile, const QString &password)
{
    m_connectingTimer.start();
    const auto attempt = m_attempt;
    m_session = m_dependencies.createSession(profile, password);
    connect(m_session.get(), &ConnectionSession::opened, this, [this, attempt] {
        if (attempt != m_attempt)
            return;
        completeConnection(attempt, true);
    });
    connect(m_session.get(), &ConnectionSession::failed, this,
            [this, attempt](const QString &error) {
                if (attempt != m_attempt)
                    return;
                m_connectionError = error;
            });
    connect(m_session.get(), &ConnectionSession::finished, this, [this, attempt] {
        m_session.reset();
        if (attempt == m_attempt && !m_connectionError.isEmpty()) {
            completeConnection(attempt, false, m_connectionError);
        } else {
            m_connecting = false;
            m_introWindow->setConnecting(false);
        }
        m_connectionError.clear();
    });
    m_session->start();
}

void ApplicationController::completeConnection(quint64 attempt, bool success, const QString &error)
{
    constexpr int MinimumConnectingMs = 450;
    const int delay = int(std::max<qint64>(0, MinimumConnectingMs - m_connectingTimer.elapsed()));
    QTimer::singleShot(delay, this, [this, attempt, success, error] {
        if (attempt != m_attempt)
            return;
        m_connecting = false;
        m_introWindow->setConnecting(false);
        if (!success) {
            m_introWindow->showConnectionError(error);
            return;
        }
        if (!m_session)
            return;
        m_mainWindow = std::make_unique<MainWindow>(m_session.get());
        connect(m_mainWindow.get(), &MainWindow::closeWorkspaceRequested, this,
                &ApplicationController::closeWorkspace, Qt::QueuedConnection);
        m_mainWindow->show();
        m_introWindow->hide();
    });
}

void ApplicationController::cancelConnection()
{
    ++m_attempt;
    if (m_passwordDialog) {
        auto *dialog = m_passwordDialog;
        m_passwordDialog = nullptr;
        dialog->reject();
    }
    if (m_session) {
        m_session->stop();
        return;
    }
    m_connecting = false;
    m_introWindow->setConnecting(false);
}

void ApplicationController::closeWorkspace()
{
    if (!m_mainWindow)
        return;
    m_introWindow->show();
    m_mainWindow.reset();
    if (m_session) {
        m_connecting = true;
        m_introWindow->setConnecting(true);
        m_session->stop();
    }
    m_introWindow->raise();
    m_introWindow->activateWindow();
}
