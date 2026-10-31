#pragma once

#include <QMainWindow>

class AppWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AppWindow(QWidget *parent = nullptr);

protected:
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
#if defined(Q_OS_WIN)
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#endif

private:
    bool m_backdropAvailable = false;
    bool m_effectsReady = false;

    virtual void buildLayout() = 0;
};
