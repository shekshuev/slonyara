#pragma once

#include "appwindow.h"

class IntroWindow : public AppWindow
{
    Q_OBJECT

public:
    explicit IntroWindow(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void buildLayout() override;
    QWidget *m_leftPanel = nullptr;
};
