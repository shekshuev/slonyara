#pragma once

#include <QDialog>

class StatusDetailsDialog final : public QDialog
{
    Q_OBJECT
public:
    explicit StatusDetailsDialog(const QString &summary, const QString &details,
                                 QWidget *parent = nullptr);

protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
#if defined(Q_OS_WIN)
    bool nativeEvent(const QByteArray &type, void *message, qintptr *result) override;
#endif

private:
    void anchorToParent();
    bool m_effectsReady = false;
    bool m_backdropAvailable = false;
};
