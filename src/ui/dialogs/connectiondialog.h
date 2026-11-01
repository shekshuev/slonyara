#pragma once

#include "../../core/connections/connectionprofile.h"
#include <QDialog>

class QLineEdit;

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
    QLineEdit *m_name;
    QLineEdit *m_host;
    QLineEdit *m_port;
    QLineEdit *m_database;
    QLineEdit *m_user;
    QLineEdit *m_password;
    bool m_effectsReady = false;
    bool m_backdropAvailable = false;
};
