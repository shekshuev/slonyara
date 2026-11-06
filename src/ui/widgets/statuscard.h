#pragma once

#include <QFrame>
#include "../../utils/theme.h"

class QLabel;
class QPushButton;

class StatusCard final : public QFrame
{
    Q_OBJECT
public:
    explicit StatusCard(QWidget *parent = nullptr);
    void setStatus(const QString &message, Theme::ColorRole role, const QString &details = {});
    void clear();

private:
    QLabel *m_message;
    QPushButton *m_detailsButton;
    QString m_details;
};
