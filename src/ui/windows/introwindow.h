#pragma once

#include "appwindow.h"
#include "../../core/connections/connectionstore.h"

class QStandardItemModel;
class QListView;
class QPushButton;
class StatusCard;

class IntroWindow : public AppWindow
{
    Q_OBJECT

public:
    explicit IntroWindow(QWidget *parent = nullptr);
    void setConnecting(bool connecting);
    void showConnectionError(const QString &details);

signals:
    void connectionRequested(const ConnectionProfile &profile);
    void connectionCancelled();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void buildLayout() override;
    void createConnection();
    void editConnection();
    void openSelectedConnection();
    void reloadConnections();
    ConnectionStore m_connectionStore;
    QStandardItemModel *m_connectionsModel = nullptr;
    QWidget *m_leftPanel = nullptr;
    QListView *m_connectionsView = nullptr;
    QPushButton *m_connectButton = nullptr;
    QPushButton *m_editButton = nullptr;
    QPushButton *m_createButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    StatusCard *m_status = nullptr;
    bool m_connecting = false;
};
