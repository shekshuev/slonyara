#pragma once

#include "appwindow.h"
#include "../../core/connections/connectionstore.h"

class QStandardItemModel;
class QListView;

class IntroWindow : public AppWindow
{
    Q_OBJECT

public:
    explicit IntroWindow(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void buildLayout() override;
    void createConnection();
    void editConnection();
    void reloadConnections();
    ConnectionStore m_connectionStore;
    QStandardItemModel *m_connectionsModel = nullptr;
    QWidget *m_leftPanel = nullptr;
    QListView *m_connectionsView = nullptr;
};
