#pragma once

#include "appwindow.h"
class ConnectionSession;

class MainWindow : public AppWindow
{
    Q_OBJECT

public:
    explicit MainWindow(ConnectionSession *session, QWidget *parent = nullptr);

signals:
    void closeWorkspaceRequested();

private:
    void buildLayout() override;
    ConnectionSession *m_session;
};
