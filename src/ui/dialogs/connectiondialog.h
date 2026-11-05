#pragma once

#include "../../core/connections/connectionprofile.h"
#include <QDialog>

class QLineEdit;
class QPushButton;
class StatusCard;
class QCheckBox;
class QDialogButtonBox;

class ConnectionDialog final : public QDialog
{
    Q_OBJECT
public:
    explicit ConnectionDialog(QWidget *parent, const ConnectionProfile *profile = nullptr);
    void completeSave(bool success);
    void reject() override;

signals:
    void connectionSubmitted(const ConnectionProfile &profile);

protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
#if defined(Q_OS_WIN)
    bool nativeEvent(const QByteArray &type, void *message, qintptr *result) override;
#endif

private:
    void anchorToParent();
    void submit();
    bool validate();
    ConnectionProfile toProfile();
    void testConnection();
    void loadPassword();
    void setBusy(bool busy);
    QLineEdit *m_name;
    QLineEdit *m_host;
    QLineEdit *m_port;
    QLineEdit *m_database;
    QLineEdit *m_user;
    QLineEdit *m_password;
    QPushButton *m_testButton = nullptr;
    StatusCard *m_statusCard = nullptr;
    QCheckBox *m_savePassword = nullptr;
    QDialogButtonBox *m_buttons = nullptr;
    bool m_busy = false;
    bool m_testing = false;
    bool m_secretUnavailable = false;
    bool m_secretReadFailed = false;
    bool m_passwordEdited = false;
    bool m_hadPassword = false;
    bool m_secretChanged = false;
    QString m_loadedPassword;
    quint64 m_formRevision = 0;
    QUuid m_connectionId = QUuid::createUuid();
    bool m_effectsReady = false;
    bool m_backdropAvailable = false;
};
