#include "statuscard.h"
#include "../dialogs/statusdetailsdialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

StatusCard::StatusCard(QWidget *parent) : QFrame(parent)
{
    setObjectName(QStringLiteral("statusCard"));
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(12);
    m_message = new QLabel(this);
    m_message->setTextFormat(Qt::PlainText);
    Theme::applyFor(m_message);
    m_detailsButton = new QPushButton(tr("Details…"), this);
    m_detailsButton->setFlat(true);
    m_detailsButton->setAutoDefault(false);
    m_detailsButton->setCursor(Qt::PointingHandCursor);
    layout->addWidget(m_message, 1);
    layout->addWidget(m_detailsButton);
    auto policy = sizePolicy();
    policy.setRetainSizeWhenHidden(true);
    setSizePolicy(policy);
    m_message->setText(QStringLiteral(" "));
    setFixedHeight(sizeHint().height());
    connect(m_detailsButton, &QPushButton::clicked, this, [this] {
        auto *dialog = new StatusDetailsDialog(m_message->text(), m_details, window());
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->open();
    });
    clear();
}

void StatusCard::setStatus(const QString &message, Theme::ColorRole role,
                           const QString &details)
{
    m_message->setText(message);
    m_details = details;
    m_detailsButton->setVisible(!details.isEmpty());
    Theme::applyFor(this, role);
    show();
}

void StatusCard::clear()
{
    m_details.clear();
    hide();
}
