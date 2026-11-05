#pragma once

#include <QObject>
#include <memory>

class IntroWindow;
class MainWindow;
struct ConnectionProfile;

class ApplicationController final : public QObject
{
    Q_OBJECT

public:
    explicit ApplicationController(QObject *parent = nullptr);
    ~ApplicationController() override;
    void start();

private:
    void openWorkspace(const ConnectionProfile &profile);
    void closeWorkspace();

    std::unique_ptr<IntroWindow> m_introWindow;
    std::unique_ptr<MainWindow> m_mainWindow;
};
