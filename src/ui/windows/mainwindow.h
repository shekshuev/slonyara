#pragma once

#include "appwindow.h"
#include "../../core/connections/connectionprofile.h"

class MainWindow : public AppWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const ConnectionProfile &profile, QWidget *parent = nullptr);

signals:
    void closeWorkspaceRequested();

private:
    void buildLayout() override;
};
