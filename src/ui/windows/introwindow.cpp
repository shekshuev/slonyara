#include "introwindow.h"

IntroWindow::IntroWindow(QWidget *parent)
    : AppWindow(parent)
{
    setFixedSize(640, 480);
    setWindowFlags((windowFlags() | Qt::CustomizeWindowHint | Qt::WindowTitleHint
                    | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
                   & ~Qt::WindowMinimizeButtonHint & ~Qt::WindowMaximizeButtonHint);

    buildLayout();
}

void IntroWindow::buildLayout() {}
