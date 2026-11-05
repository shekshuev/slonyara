#include "ui/windows/introwindow.h"
#include "ui/dialogs/connectiondialog.h"
#include "ui/widgets/statuscard.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QFile>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPointer>
#include <QPushButton>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <memory>

class IntroWindowTest final : public QObject
{
    Q_OBJECT
private:
    std::unique_ptr<QTemporaryDir> m_settings;
    std::unique_ptr<IntroWindow> m_window;
    ConnectionProfile m_first, m_second;
    QList<ConnectionProfile> m_requested;
    QListView *m_list = nullptr;
    QLineEdit *m_search = nullptr;
    QPushButton *m_connect = nullptr, *m_edit = nullptr, *m_create = nullptr, *m_cancel = nullptr;

    QPushButton *button(const char *text) const {
        for (auto *button : m_window->findChildren<QPushButton *>())
            if (button->text() == IntroWindow::tr(text)) return button;
        return nullptr;
    }
    void createWindow() {
        m_window = std::make_unique<IntroWindow>();
        connect(m_window.get(), &IntroWindow::connectionRequested, this,
                [this](const auto &profile) { m_requested << profile; });
        m_window->show();
        m_list = m_window->findChild<QListView *>();
        m_search = m_window->findChild<QLineEdit *>();
        m_connect = button("Connect"); m_edit = button("Edit");
        m_create = button("New Connection"); m_cancel = button("Cancel");
        QVERIFY(m_list && m_search && m_connect && m_edit && m_create && m_cancel);
    }
    void selectRow(int row) {
        const auto index = m_list->model()->index(row, 0);
        QVERIFY(index.isValid());
        m_list->setCurrentIndex(index);
        m_list->scrollTo(index);
    }
    ConnectionDialog *editor() const { return m_window->findChild<ConnectionDialog *>(); }
    void saveEditor() {
        QVERIFY(editor());
        auto *buttons = editor()->findChild<QDialogButtonBox *>();
        QVERIFY(buttons);
        QTest::mouseClick(buttons->button(QDialogButtonBox::Save), Qt::LeftButton);
    }

private slots:
    void initTestCase() {
        QApplication::setQuitOnLastWindowClosed(false);
        QCoreApplication::setOrganizationName("SlonyaraTests");
        QCoreApplication::setApplicationName("IntroWindowTest");
        QSettings::setDefaultFormat(QSettings::IniFormat);
    }
    void init() {
        m_settings = std::make_unique<QTemporaryDir>();
        QVERIFY(m_settings->isValid());
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings->path());
        QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, m_settings->path());
        m_requested.clear();
        m_first = ConnectionProfile{};
        m_first.name = "Local"; m_first.host = "localhost";
        m_first.database = "app_dev"; m_first.user = "developer";
        m_second = ConnectionProfile{};
        m_second.name = "Reporting"; m_second.host = "analytics.example.test";
        m_second.port = 15432; m_second.database = "warehouse"; m_second.user = "reader";
        ConnectionStore store;
        store.addConnection(m_first); store.addConnection(m_second);
        QVERIFY(store.save());
        createWindow();
    }
    void cleanup() {
        m_window.reset();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        m_settings.reset();
    }
    void loadsProfilesWithoutSelectingOne() {
        QCOMPARE(m_list->model()->rowCount(), 2);
        QCOMPARE(m_list->model()->index(0, 0).data().toString(), m_first.name);
        QCOMPARE(m_list->model()->index(1, 0).data().toString(), m_second.name);
        QVERIFY(!m_connect->isEnabled());
        QVERIFY(!m_edit->isEnabled());
        QVERIFY(m_create->isEnabled());
        QVERIFY(!m_cancel->isVisible());
        QVERIFY(m_requested.isEmpty());
    }
    void emptyStoreShowsEmptyState() {
        m_window.reset();
        QSettings settings;
        settings.clear(); settings.sync();
        createWindow();
        QCOMPARE(m_list->model()->rowCount(), 0);
        QVERIFY(!m_list->isVisible());
        bool emptyLabelVisible = false;
        for (auto *label : m_window->findChildren<QLabel *>())
            if (label->text() == IntroWindow::tr("No connections yet")) emptyLabelVisible = label->isVisible();
        QVERIFY(emptyLabelVisible);
        QVERIFY(!m_connect->isEnabled());
        QVERIFY(!m_edit->isEnabled());
        QVERIFY(m_create->isEnabled());
    }
    void searchMatchesProfileFields_data() {
        QTest::addColumn<QString>("query");
        QTest::newRow("name-case-insensitive") << "rEpOrTiNg";
        QTest::newRow("host") << "analytics.example";
        QTest::newRow("port") << "15432";
        QTest::newRow("database") << "warehouse";
        QTest::newRow("user") << "reader";
    }
    void searchMatchesProfileFields() {
        QFETCH(QString, query);
        m_search->setText(query);
        QCOMPARE(m_list->model()->rowCount(), 1);
        QCOMPARE(m_list->model()->index(0, 0).data().toString(), m_second.name);
        selectRow(0);
        QTest::mouseClick(m_connect, Qt::LeftButton);
        QCOMPARE(m_requested.size(), 1);
        QCOMPARE(m_requested.first().id, m_second.id);
        QCOMPARE(m_requested.first().host, m_second.host);
        QCOMPARE(m_requested.first().port, m_second.port);
        QCOMPARE(m_requested.first().database, m_second.database);
        QCOMPARE(m_requested.first().user, m_second.user);
    }
    void searchWithoutMatchesAndClearing() {
        selectRow(0);
        m_search->setText("no-such-connection");
        QCOMPARE(m_list->model()->rowCount(), 0);
        QVERIFY(!m_connect->isEnabled());
        QVERIFY(!m_edit->isEnabled());
        QTest::keyClick(m_list, Qt::Key_Return);
        QVERIFY(m_requested.isEmpty());
        m_search->clear();
        QCOMPARE(m_list->model()->rowCount(), 2);
    }
    void activation_data() {
        QTest::addColumn<int>("method");
        QTest::newRow("connect-button") << 0;
        QTest::newRow("return") << 1;
        QTest::newRow("keypad-enter") << 2;
        QTest::newRow("double-click") << 3;
    }
    void activation() {
        QFETCH(int, method);
        selectRow(1);
        QVERIFY(m_connect->isEnabled());
        QVERIFY(m_edit->isEnabled());
        if (method == 0) QTest::mouseClick(m_connect, Qt::LeftButton);
        else if (method == 1) QTest::keyClick(m_list, Qt::Key_Return);
        else if (method == 2) QTest::keyClick(m_list, Qt::Key_Enter);
        else {
            const auto point = m_list->visualRect(m_list->currentIndex()).center();
            QTest::mouseClick(m_list->viewport(), Qt::LeftButton, Qt::NoModifier, point);
            QTest::mouseDClick(m_list->viewport(), Qt::LeftButton, Qt::NoModifier, point);
        }
        QCOMPARE(m_requested.size(), 1);
        QCOMPARE(m_requested.first().id, m_second.id);
    }
    void singleClickOnlySelects() {
        const auto index = m_list->model()->index(0, 0);
        QTest::mouseClick(m_list->viewport(), Qt::LeftButton, Qt::NoModifier,
                          m_list->visualRect(index).center());
        QVERIFY(m_requested.isEmpty());
        QVERIFY(m_connect->isEnabled());
    }
    void noActivationWithoutSelectionOrOnAutoRepeat() {
        QTest::keyClick(m_list, Qt::Key_Return);
        QVERIFY(m_requested.isEmpty());
        selectRow(0);
        QKeyEvent repeated(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, {}, true);
        QApplication::sendEvent(m_list, &repeated);
        QVERIFY(m_requested.isEmpty());
        m_list->selectionModel()->clearSelection();
        QVERIFY(!m_connect->isEnabled());
        QVERIFY(!m_edit->isEnabled());
    }
    void connectingBlocksActionsAndEmitsCancellation() {
        selectRow(1);
        m_window->setConnecting(true);
        QVERIFY(!m_connect->isEnabled());
        QVERIFY(!m_edit->isEnabled());
        QVERIFY(!m_create->isEnabled());
        QVERIFY(!m_list->isEnabled());
        QVERIFY(m_cancel->isVisible());
        QTest::keyClick(m_list, Qt::Key_Return);
        QTest::mouseClick(m_connect, Qt::LeftButton);
        QVERIFY(m_requested.isEmpty());
        QSignalSpy cancelled(m_window.get(), &IntroWindow::connectionCancelled);
        QTest::mouseClick(m_cancel, Qt::LeftButton);
        QCOMPARE(cancelled.count(), 1);
        m_window->setConnecting(false);
        QVERIFY(m_connect->isEnabled());
        QVERIFY(m_edit->isEnabled());
        QVERIFY(m_create->isEnabled());
        QVERIFY(m_list->isEnabled());
        QVERIFY(!m_cancel->isVisible());
        QCOMPARE(m_list->currentIndex().row(), 1);
    }
    void errorPersistsUntilNextAttempt() {
        auto *card = m_window->findChild<StatusCard *>();
        QVERIFY(card);
        m_window->showConnectionError("Server refused connection");
        QVERIFY(card->isVisible());
        QVERIFY(card->findChild<QPushButton *>()->isVisible());
        m_window->setConnecting(false);
        QVERIFY(card->isVisible());
        m_window->setConnecting(true);
        QVERIFY(!card->isVisible());
    }
    void createPersistsAndUpdatesList() {
        QTest::mouseClick(m_create, Qt::LeftButton);
        QVERIFY(editor());
        editor()->findChild<QLineEdit *>("connectionName")->setText("New profile");
        QPointer<ConnectionDialog> dialog = editor();
        saveEditor();
        QTRY_VERIFY(dialog.isNull());
        QCOMPARE(m_list->model()->rowCount(), 3);
        ConnectionStore loaded;
        QCOMPARE(loaded.connections().size(), 3);
        QCOMPARE(loaded.connections().last().name, QString("New profile"));
        QVERIFY(!loaded.connections().last().id.isNull());
        QVERIFY(m_list->isVisible());
    }
    void editFilteredProfilePreservesIdAndSelection() {
        m_search->setText("Reporting");
        selectRow(0);
        QTest::mouseClick(m_edit, Qt::LeftButton);
        QVERIFY(editor());
        QCOMPARE(editor()->findChild<QLineEdit *>("connectionHost")->text(), m_second.host);
        editor()->findChild<QLineEdit *>("connectionName")->setText("Reporting updated");
        QPointer<ConnectionDialog> dialog = editor();
        saveEditor();
        QTRY_VERIFY(dialog.isNull());
        QCOMPARE(m_list->model()->rowCount(), 1);
        QCOMPARE(m_list->currentIndex().data().toString(), QString("Reporting updated"));
        ConnectionStore loaded;
        QCOMPARE(loaded.connections().size(), 2);
        QCOMPARE(loaded.connections().at(0).name, m_first.name);
        QCOMPARE(loaded.connections().at(1).id, m_second.id);
        QTest::mouseClick(m_connect, Qt::LeftButton);
        QCOMPARE(m_requested.size(), 1);
        QCOMPARE(m_requested.first().id, m_second.id);
        QCOMPARE(m_requested.first().name, QString("Reporting updated"));
    }
    void cancelledDialogLeavesStoreUnchanged_data() {
        QTest::addColumn<bool>("edit");
        QTest::newRow("create") << false;
        QTest::newRow("edit") << true;
    }
    void cancelledDialogLeavesStoreUnchanged() {
        QFETCH(bool, edit);
        selectRow(0);
        QTest::mouseClick(edit ? m_edit : m_create, Qt::LeftButton);
        QVERIFY(editor());
        editor()->findChild<QLineEdit *>("connectionName")->setText("Discarded");
        editor()->reject();
        QCOMPARE(m_list->model()->rowCount(), 2);
        ConnectionStore loaded;
        QCOMPARE(loaded.connections().size(), 2);
        QCOMPARE(loaded.connections().first().name, m_first.name);
    }
    void failedSaveRollsBackAndCanBeRetried_data() {
        QTest::addColumn<bool>("edit");
        QTest::newRow("create") << false;
        QTest::newRow("edit") << true;
    }
    void failedSaveRollsBackAndCanBeRetried() {
        QFETCH(bool, edit);
        selectRow(0);
        QTest::mouseClick(edit ? m_edit : m_create, Qt::LeftButton);
        QVERIFY(editor());
        editor()->findChild<QLineEdit *>("connectionName")->setText("Retry profile");
        const auto blockedPath = m_settings->filePath("blocked");
        QFile blocker(blockedPath);
        QVERIFY(blocker.open(QIODevice::WriteOnly)); blocker.close();
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, blockedPath);
        saveEditor();
        QVERIFY(editor()->isVisible());
        QVERIFY(editor()->findChild<StatusCard *>()->isVisible());
        QCOMPARE(m_list->model()->rowCount(), 2);
        QCOMPARE(m_list->model()->index(0, 0).data().toString(), m_first.name);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings->path());
        saveEditor();
        QCOMPARE(m_list->model()->rowCount(), edit ? 2 : 3);
        ConnectionStore loaded;
        QCOMPARE(loaded.connections().size(), edit ? 2 : 3);
        QCOMPARE(loaded.connections().at(edit ? 0 : 2).name, QString("Retry profile"));
        if (edit) QCOMPARE(loaded.connections().first().id, m_first.id);
    }
};

QTEST_MAIN(IntroWindowTest)
#include "introwindowtest.moc"
