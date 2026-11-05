#include "ui/dialogs/statusdetailsdialog.h"
#include "ui/widgets/statuscard.h"

#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
#include <QVBoxLayout>

class StatusDetailsDialogTest final : public QObject
{
    Q_OBJECT
private:
    static QPushButton *button(StatusDetailsDialog &dialog, QDialogButtonBox::ButtonRole role) {
        auto *box = dialog.findChild<QDialogButtonBox *>();
        if (!box) return nullptr;
        for (auto *candidate : box->buttons())
            if (box->buttonRole(candidate) == role)
                return qobject_cast<QPushButton *>(candidate);
        return nullptr;
    }
    static QPoint centeredPosition(const QWidget &parent, const StatusDetailsDialog &dialog) {
#if defined(Q_OS_WIN)
        const auto size = dialog.frameGeometry().size();
#else
        const auto size = dialog.size();
#endif
        return parent.mapToGlobal(QPoint((parent.width() - size.width()) / 2,
                                        (parent.height() - size.height()) / 2));
    }

private slots:
    void initTestCase() {
        QApplication::setQuitOnLastWindowClosed(false);
    }
    void content_data() {
        QTest::addColumn<QString>("summary");
        QTest::addColumn<QString>("details");
        QTest::newRow("plain") << "Connection failed" << "Server refused the connection";
        QTest::newRow("empty") << "" << "";
        QTest::newRow("unicode-multiline") << QString::fromUtf8("Ошибка подключения 🐘")
            << QString::fromUtf8("Сервер недоступен\nПользователь: тест\nПовторите попытку.");
        QTest::newRow("literal-markup") << "<b>Error & warning</b>"
            << "<html><script>example</script></html>\nA & B";
        QTest::newRow("long") << QString(1000, 'S') << QString("Long diagnostic line\n").repeated(1000);
    }
    void content() {
        QFETCH(QString, summary);
        QFETCH(QString, details);
        StatusDetailsDialog dialog(summary, details);
        auto *title = dialog.findChild<QLabel *>();
        auto *text = dialog.findChild<QPlainTextEdit *>();
        QVERIFY(title);
        QVERIFY(text);
        QCOMPARE(title->text(), summary);
        QCOMPARE(title->textFormat(), Qt::PlainText);
        QCOMPARE(text->toPlainText(), details);
        QVERIFY(text->isReadOnly());
        dialog.show();
        text->setFocus();
        QTest::keyClicks(text, "overwrite");
        QCOMPARE(text->toPlainText(), details);
    }
    void copyPreservesExactDetailsAndKeepsDialogOpen_data() {
        QTest::addColumn<QString>("details");
        QTest::newRow("multiline") << QString::fromUtf8("Ошибка 'quoted' \\ path 🐘\nSecond line\n");
        QTest::newRow("empty") << QString{};
    }
    void copyPreservesExactDetailsAndKeepsDialogOpen() {
        QFETCH(QString, details);
        QApplication::clipboard()->setText("old clipboard contents");
        StatusDetailsDialog dialog("Summary must not be copied", details);
        dialog.show();
        auto *copy = button(dialog, QDialogButtonBox::ActionRole);
        QVERIFY(copy);
        QSignalSpy finished(&dialog, &QDialog::finished);
        QTest::mouseClick(copy, Qt::LeftButton);
        QCOMPARE(QApplication::clipboard()->text(), details);
        QVERIFY(dialog.isVisible());
        QCOMPARE(finished.count(), 0);
    }
    void closeButtonRejectsDialog() {
        StatusDetailsDialog dialog("Error", "Details");
        dialog.show();
        auto *close = button(dialog, QDialogButtonBox::RejectRole);
        QVERIFY(close);
        QSignalSpy rejected(&dialog, &QDialog::rejected);
        QTest::mouseClick(close, Qt::LeftButton);
        QCOMPARE(rejected.count(), 1);
        QVERIFY(!dialog.isVisible());
        QCOMPARE(dialog.result(), int(QDialog::Rejected));
    }
    void escapeRejectsDialog() {
        StatusDetailsDialog dialog("Error", "Details");
        dialog.show();
        QSignalSpy rejected(&dialog, &QDialog::rejected);
        QTest::keyClick(&dialog, Qt::Key_Escape);
        QCOMPARE(rejected.count(), 1);
        QVERIFY(!dialog.isVisible());
    }
    void worksWithoutParent() {
        StatusDetailsDialog dialog("Error", "Details");
        dialog.show();
        QVERIFY(dialog.isVisible());
        QVERIFY(!dialog.parentWidget());
        dialog.close();
        QVERIFY(!dialog.isVisible());
    }
    void staysCenteredWhenParentMovesAndResizes() {
        QWidget parent;
        parent.resize(900, 700);
        parent.move(50, 50);
        parent.show();
        StatusDetailsDialog dialog("Error", "Details", &parent);
        dialog.open();
        QCOMPARE(dialog.windowModality(), Qt::WindowModal);
        QTRY_COMPARE(dialog.pos(), centeredPosition(parent, dialog));
        parent.move(100, 120);
        QTRY_COMPARE(dialog.pos(), centeredPosition(parent, dialog));
        parent.resize(1000, 800);
        QTRY_COMPARE(dialog.pos(), centeredPosition(parent, dialog));
    }
    void cardOpensCurrentDetailsAndDeletesDialogOnClose() {
        QWidget parent;
        auto *layout = new QVBoxLayout(&parent);
        auto *card = new StatusCard(&parent);
        layout->addWidget(card);
        parent.show();
        card->setStatus("Old summary", Theme::ColorRole::Error, "Old details");
        card->setStatus("Current summary", Theme::ColorRole::Error, "Current details");
        auto *detailsButton = card->findChild<QPushButton *>();
        QVERIFY(detailsButton);
        QVERIFY(detailsButton->isVisible());
        QTest::mouseClick(detailsButton, Qt::LeftButton);
        QPointer<StatusDetailsDialog> dialog = parent.findChild<StatusDetailsDialog *>();
        QVERIFY(dialog);
        QVERIFY(dialog->isVisible());
        QCOMPARE(dialog->parentWidget(), &parent);
        QCOMPARE(dialog->windowModality(), Qt::WindowModal);
        QCOMPARE(dialog->findChild<QLabel *>()->text(), QString("Current summary"));
        QCOMPARE(dialog->findChild<QPlainTextEdit *>()->toPlainText(), QString("Current details"));
        dialog->close();
        QTRY_VERIFY(dialog.isNull());
    }
    void cardWithoutDetailsHidesDetailsAction() {
        QWidget parent;
        StatusCard card(&parent);
        parent.show();
        card.setStatus("With details", Theme::ColorRole::Error, "Details");
        auto *detailsButton = card.findChild<QPushButton *>();
        QVERIFY(detailsButton);
        QVERIFY(detailsButton->isVisible());
        card.setStatus("No details", Theme::ColorRole::Info);
        QVERIFY(!detailsButton->isVisible());
        QVERIFY(parent.findChildren<StatusDetailsDialog *>().isEmpty());
    }
};

QTEST_MAIN(StatusDetailsDialogTest)
#include "statusdetailsdialogtest.moc"
