#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <memory>

class IntroWindow;
class MainWindow;
class ConnectionSession;
class QInputDialog;
struct ConnectionProfile;

class ApplicationController final : public QObject
{
    Q_OBJECT

public:
    explicit ApplicationController(QObject *parent = nullptr);
    ~ApplicationController() override;
    void start();

private:
    void openWorkspace(const ConnectionProfile &profile);
    void closeWorkspace();
    void connectProfile(const ConnectionProfile &profile, const QString &password);
    void cancelConnection();
    void requestPassword(const ConnectionProfile &profile);
    void completeConnection(quint64 attempt, bool success, const QString &error = {});

    std::unique_ptr<IntroWindow> m_introWindow;
    std::unique_ptr<MainWindow> m_mainWindow;
    std::unique_ptr<ConnectionSession> m_session;
    QInputDialog *m_passwordDialog = nullptr;
    quint64 m_attempt = 0;
    bool m_connecting = false;
    QString m_connectionError;
    QElapsedTimer m_connectingTimer;
};
