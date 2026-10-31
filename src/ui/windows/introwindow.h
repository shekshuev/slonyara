#pragma once

#include "appwindow.h"

class IntroWindow : public AppWindow
{
    Q_OBJECT

public:
    explicit IntroWindow(QWidget *parent = nullptr);

private:
    void buildLayout() override;
};
