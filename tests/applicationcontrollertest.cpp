#include "applicationcontroller.h"
#include "core/connections/connectionsession.h"
#include "ui/windows/introwindow.h"
#include "ui/windows/mainwindow.h"
#include "ui/widgets/statuscard.h"

#include <QApplication>
#include <QInputDialog>
#include <QLabel>
#include <QPointer>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

class FakeSession final : public ConnectionSession
{
public:
    using ConnectionSession::ConnectionSession;
    int starts = 0;
    int stops = 0;
    void start() override { ++starts; }
    void stop() override { ++stops; }
    void succeed() { emit opened(); }
    void fail(const QString &error) { emit failed(error); finish(); }
    void finish()
    {
        QTimer::singleShot(0, this, [this] { emit finished(); });
    }
};

class ApplicationControllerTest final : public QObject
{
    Q_OBJECT
private:
    QTemporaryDir m_settings;
    std::unique_ptr<ApplicationController> m_controller;
    QPointer<IntroWindow> m_intro;
    QPointer<FakeSession> m_session;
    ConnectionSecrets::Callback m_passwordResult;
    ConnectionProfile m_profile;
    QString m_password;
    QUuid m_requestedId;
    int m_created = 0;

    MainWindow *mainWindow() const {
        for (auto *widget : QApplication::topLevelWidgets())
            if (auto *window = qobject_cast<MainWindow *>(widget)) return window;
        return nullptr;
    }
    StatusCard *card() const { return m_intro->findChild<StatusCard *>(); }
    void request() { m_intro->connectionRequested(m_profile); }
    void supplyPassword() {
        QVERIFY(bool(m_passwordResult));
        auto callback = std::move(m_passwordResult);
        callback({true, true, QStringLiteral("secret"), {}});
    }
    bool errorVisible() const { return card() && card()->isVisible(); }

private slots:
    void initTestCase() {
        QVERIFY(m_settings.isValid());
        QCoreApplication::setOrganizationName("SlonyaraTests");
        QCoreApplication::setApplicationName("ApplicationControllerTest");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
        QStandardPaths::setTestModeEnabled(true);
        QApplication::setQuitOnLastWindowClosed(false);
    }
    void init() {
        m_created = 0;
        m_password.clear();
        m_passwordResult = {};
        m_profile = ConnectionProfile{};
        m_profile.name = "Test server";
        m_profile.savePassword = true;
        ApplicationController::Dependencies dependencies;
        dependencies.readPassword = [this](const QUuid &id, QObject *, auto callback) {
            m_requestedId = id;
            m_passwordResult = std::move(callback);
        };
        dependencies.createSession = [this](const ConnectionProfile &profile, const QString &password) {
            ++m_created;
            m_password = password;
            auto session = std::make_unique<FakeSession>(profile);
            m_session = session.get();
            return session;
        };
        m_controller = std::make_unique<ApplicationController>(std::move(dependencies));
        m_controller->start();
        for (auto *widget : QApplication::topLevelWidgets())
            if (auto *intro = qobject_cast<IntroWindow *>(widget)) m_intro = intro;
        QVERIFY(m_intro);
        QVERIFY(m_intro->isVisible());
    }
    void cleanup() {
        m_controller.reset();
        m_passwordResult = {};
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(!mainWindow());
        QVERIFY(!m_session);
    }
    void startsWithIntroOnly() {
        QVERIFY(!mainWindow());
        QCOMPARE(m_created, 0);
    }
    void opensAfterSuccessAndMinimumDelay() {
        request();
        QCOMPARE(m_requestedId, m_profile.id);
        QVERIFY(!mainWindow());
        supplyPassword();
        QCOMPARE(m_session->starts, 1);
        QCOMPARE(m_password, QString("secret"));
        QCOMPARE(m_session->profile().id, m_profile.id);
        QElapsedTimer elapsed;
        elapsed.start();
        m_session->succeed();
        QTest::qWait(100);
        QVERIFY(!mainWindow());
        QTRY_VERIFY(mainWindow());
        QVERIFY(elapsed.elapsed() >= 400);
        QVERIFY(mainWindow()->isVisible());
        QVERIFY(!m_intro->isVisible());
        QVERIFY(m_session);
    }
    void slowSuccessHasNoSecondMinimumDelay() {
        request();
        supplyPassword();
        QTest::qWait(500);
        m_session->succeed();
        QTRY_VERIFY_WITH_TIMEOUT(mainWindow(), 200);
    }
    void failureKeepsErrorVisibleAndAllowsRetry() {
        request();
        supplyPassword();
        m_session->fail("authentication failed");
        QTRY_VERIFY(errorVisible());
        QVERIFY(m_intro->isVisible());
        QVERIFY(!mainWindow());
        QVERIFY(!m_session);
        QTest::qWait(100);
        QVERIFY(errorVisible());
        request();
        QVERIFY(!errorVisible());
        supplyPassword();
        QCOMPARE(m_created, 2);
    }
    void duplicateRequestsCreateOneSession() {
        request();
        request();
        supplyPassword();
        request();
        QCOMPARE(m_created, 1);
        QCOMPARE(m_session->starts, 1);
    }
    void cancellationIgnoresLateSuccess() {
        request();
        supplyPassword();
        m_intro->connectionCancelled();
        QCOMPARE(m_session->stops, 1);
        m_session->succeed();
        m_session->finish();
        QTRY_VERIFY(!m_session);
        QTest::qWait(500);
        QVERIFY(!mainWindow());
        QVERIFY(!errorVisible());
        QVERIFY(m_intro->isVisible());
    }
    void cancellationDuringTransitionPreventsOpening() {
        request();
        supplyPassword();
        m_session->succeed();
        m_intro->connectionCancelled();
        m_session->finish();
        QTRY_VERIFY(!m_session);
        QTest::qWait(500);
        QVERIFY(!mainWindow());
    }
    void latePasswordResultCannotStartSession() {
        request();
        auto oldCallback = std::move(m_passwordResult);
        m_intro->connectionCancelled();
        request();
        oldCallback({true, true, "old secret", {}});
        QCOMPARE(m_created, 0);
        supplyPassword();
        QCOMPARE(m_created, 1);
        QCOMPARE(m_password, QString("secret"));
    }
    void keychainErrorDoesNotStartSession() {
        request();
        m_passwordResult({false, false, {}, "Keychain unavailable"});
        QTRY_VERIFY(errorVisible());
        QCOMPARE(m_created, 0);
        QVERIFY(!mainWindow());
    }
    void missingPasswordCanBeEntered() {
        request();
        m_passwordResult({true, false, {}, {}});
        auto *dialog = m_intro->findChild<QInputDialog *>();
        QVERIFY(dialog);
        dialog->setTextValue("entered secret");
        dialog->accept();
        QCOMPARE(m_password, QString("entered secret"));
        QCOMPARE(m_created, 1);
    }
    void passwordDialogCancellationDoesNotStartSession() {
        m_profile.savePassword = false;
        request();
        auto *dialog = m_intro->findChild<QInputDialog *>();
        QVERIFY(dialog);
        dialog->reject();
        QCOMPARE(m_created, 0);
        QVERIFY(!mainWindow());
        QVERIFY(!errorVisible());
    }
    void closingWindowDoesNotReturnToIntro() {
        request();
        supplyPassword();
        m_session->succeed();
        QTRY_VERIFY(mainWindow());
        QVERIFY(mainWindow()->close());
        QVERIFY(!m_intro->isVisible());
        QVERIFY(!mainWindow()->isVisible());
        QVERIFY(mainWindow()->testAttribute(Qt::WA_QuitOnClose));
    }
    void closeWorkspaceStopsSessionAndAllowsReopening() {
        request();
        supplyPassword();
        m_session->succeed();
        QTRY_VERIFY(mainWindow());
        mainWindow()->closeWorkspaceRequested();
        QTRY_VERIFY(!mainWindow());
        QVERIFY(m_intro->isVisible());
        QCOMPARE(m_session->stops, 1);
        m_session->finish();
        QTRY_VERIFY(!m_session);
        request();
        supplyPassword();
        m_session->succeed();
        QTRY_VERIFY(mainWindow());
        QCOMPARE(m_created, 2);
    }
};

QTEST_MAIN(ApplicationControllerTest)
#include "applicationcontrollertest.moc"
