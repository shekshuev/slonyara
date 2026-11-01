#pragma once

#include "appwindow.h"
#include "../../core/connections/connectionstore.h"

class QStandardItemModel;

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
    void reloadConnections();
    ConnectionStore m_connectionStore;
    QStandardItemModel *m_connectionsModel = nullptr;
    QWidget *m_leftPanel = nullptr;
};
