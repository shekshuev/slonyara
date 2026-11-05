#pragma once

#include "../../core/connections/connectionprofile.h"
#include <QDialog>

class QLineEdit;
class QPushButton;
class StatusCard;

class ConnectionDialog final : public QDialog
{
    Q_OBJECT
public:
    explicit ConnectionDialog(QWidget *parent);

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
    QLineEdit *m_name;
    QLineEdit *m_host;
    QLineEdit *m_port;
    QLineEdit *m_database;
    QLineEdit *m_user;
    QLineEdit *m_password;
    QPushButton *m_testButton = nullptr;
    StatusCard *m_statusCard = nullptr;
    quint64 m_formRevision = 0;
    bool m_effectsReady = false;
    bool m_backdropAvailable = false;
};
