#pragma once

#include "appwindow.h"

class MainWindow : public AppWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void buildLayout();
};
