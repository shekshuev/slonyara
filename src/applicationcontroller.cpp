#include "applicationcontroller.h"

#include "ui/windows/introwindow.h"
#include "ui/windows/mainwindow.h"

ApplicationController::ApplicationController(QObject *parent)
    : QObject(parent), m_introWindow(std::make_unique<IntroWindow>())
{
    connect(m_introWindow.get(), &IntroWindow::connectionRequested,
            this, &ApplicationController::openWorkspace);
}

ApplicationController::~ApplicationController() = default;

void ApplicationController::start()
{
    m_introWindow->show();
}

void ApplicationController::openWorkspace(const ConnectionProfile &profile)
{
    if (m_mainWindow)
        return;

    m_mainWindow = std::make_unique<MainWindow>(profile);
    connect(m_mainWindow.get(), &MainWindow::closeWorkspaceRequested,
            this, &ApplicationController::closeWorkspace, Qt::QueuedConnection);
    m_mainWindow->show();
    m_introWindow->hide();
}

void ApplicationController::closeWorkspace()
{
    if (!m_mainWindow)
        return;
    m_introWindow->show();
    m_mainWindow.reset();
    m_introWindow->raise();
    m_introWindow->activateWindow();
}
