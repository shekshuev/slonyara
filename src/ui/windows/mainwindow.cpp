#include "mainwindow.h"
#include "../../core/connections/connectionsession.h"

#include <QAction>
#include <QApplication>
#include <QMenuBar>
#include <QMenu>

MainWindow::MainWindow(ConnectionSession *session, QWidget *parent)
    : AppWindow(parent), m_session(session)
{
    setWindowTitle(tr("%1 — Slonyara").arg(session->profile().name));
    resize(1200, 750);
    buildLayout();
}

void MainWindow::buildLayout()
{
    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    auto *closeWorkspace = fileMenu->addAction(tr("Close Workspace"));
    closeWorkspace->setMenuRole(QAction::NoRole);
    connect(closeWorkspace, &QAction::triggered, this, &MainWindow::closeWorkspaceRequested);
    fileMenu->addSeparator();
    auto *quit = fileMenu->addAction(tr("Quit"));
    quit->setMenuRole(QAction::QuitRole);
    quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, qApp, &QApplication::closeAllWindows);
}
