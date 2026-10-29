#include "mainwindow.h"
#include <QApplication>
#include <QPalette>
#include <QSplitter>
#include <QTreeWidget>
#include <QTableView>
#include <QVBoxLayout>
#include <QHeaderView>


#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    enableWin32DarkMode();
    buildLayout();

    resize(1200, 750);
    setWindowTitle("Slonyara — PostgreSQL Client");
}

MainWindow::~MainWindow() = default;

void MainWindow::enableWin32DarkMode()
{
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    BOOL useDarkMode = TRUE;
    DwmSetWindowAttribute(hwnd, 20, &useDarkMode, sizeof(useDarkMode));
#endif
}


void MainWindow::buildLayout()
{
    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(4, 4, 4, 4);

    auto *splitter = new QSplitter(Qt::Horizontal, this);

    auto *tree = new QTreeWidget(this);
    tree->setHeaderLabel("Connections");
    auto *rootItem = new QTreeWidgetItem(tree, QStringList() << "local_pg_16");
    auto *dbItem = new QTreeWidgetItem(rootItem, QStringList() << "postgres");
    new QTreeWidgetItem(dbItem, QStringList() << "public (schema)");
    tree->expandAll();

    auto *table = new QTableView(this);

    splitter->addWidget(tree);
    splitter->addWidget(table);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 4);

    rootLayout->addWidget(splitter);
    setCentralWidget(central);
}