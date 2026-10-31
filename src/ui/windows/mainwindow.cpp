#include "mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : AppWindow(parent)
{
    setWindowTitle(QStringLiteral("Slonyara App"));
    resize(1200, 750);
    buildLayout();
}

void MainWindow::buildLayout()
{

}
