#include "ui/widgets/statuscard.h"

#include <QLabel>
#include <QPushButton>
#include <QTest>
#include <QVBoxLayout>

class StatusCardTest final : public QObject
{
    Q_OBJECT
private slots:
    void startsHidden()
    {
        QWidget parent;
        StatusCard card(&parent);
        parent.show();
        QVERIFY(card.isHidden());
    }
    void showsMessageAsPlainText()
    {
        QWidget parent;
        StatusCard card(&parent);
        parent.show();
        const QString message = QString::fromUtf8("<b>Ошибка 🐘</b> & details");
        card.setStatus(message, Theme::ColorRole::Error);
        QVERIFY(card.isVisible());
        auto *label = card.findChild<QLabel *>();
        QVERIFY(label);
        QCOMPARE(label->text(), message);
        QCOMPARE(label->textFormat(), Qt::PlainText);
    }
    void detailsActionTracksCurrentStatus()
    {
        QWidget parent;
        StatusCard card(&parent);
        parent.show();
        auto *details = card.findChild<QPushButton *>();
        QVERIFY(details);
        card.setStatus("Loading", Theme::ColorRole::Info);
        QVERIFY(!details->isVisible());
        card.setStatus("Failed", Theme::ColorRole::Error, "Server unavailable");
        QVERIFY(details->isVisible());
        card.setStatus("Connected", Theme::ColorRole::Success);
        QVERIFY(!details->isVisible());
        QCOMPARE(card.findChild<QLabel *>()->text(), QString("Connected"));
    }
    void clearIsRepeatableAndCardCanBeReused()
    {
        QWidget parent;
        StatusCard card(&parent);
        parent.show();
        card.setStatus("Old error", Theme::ColorRole::Error, "Old details");
        card.clear();
        card.clear();
        QVERIFY(!card.isVisible());
        card.setStatus("Ready", Theme::ColorRole::Success);
        QVERIFY(card.isVisible());
        QCOMPARE(card.findChild<QLabel *>()->text(), QString("Ready"));
        QVERIFY(!card.findChild<QPushButton *>()->isVisible());
    }
    void clearingReservesLayoutSpace()
    {
        QWidget parent;
        auto *layout = new QVBoxLayout(&parent);
        auto *card = new StatusCard(&parent);
        layout->addWidget(card);
        parent.show();
        card->setStatus("Failed", Theme::ColorRole::Error, "Details");
        layout->activate();
        const auto visibleSize = layout->sizeHint();
        card->clear();
        layout->activate();
        QCOMPARE(layout->sizeHint(), visibleSize);
    }
};

QTEST_MAIN(StatusCardTest)
#include "statuscardtest.moc"
