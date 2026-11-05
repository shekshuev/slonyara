#include "ui/dialogs/connectiondialog.h"
#include "ui/widgets/statuscard.h"

#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPromise>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
#include <memory>

class ConnectionDialogTest final : public QObject
{
    Q_OBJECT
private:
    std::unique_ptr<QWidget> m_parent;
    std::unique_ptr<ConnectionDialog> m_dialog;
    QList<ConnectionProfile> m_submitted;
    QStringList m_operations;
    QUuid m_secretId;
    QString m_writtenPassword;
    ConnectionSecrets::Callback m_secretResult;
    std::unique_ptr<QPromise<ConnectionTestResult>> m_check;
    ConnectionProfile m_checkedProfile;
    QString m_checkedPassword;
    int m_checks = 0;

    QLineEdit *field(const char *name) const { return m_dialog->findChild<QLineEdit *>(name); }
    QCheckBox *savePassword() const { return m_dialog->findChild<QCheckBox *>(); }
    QDialogButtonBox *buttons() const { return m_dialog->findChild<QDialogButtonBox *>(); }
    QPushButton *save() const { return buttons()->button(QDialogButtonBox::Save); }
    QPushButton *check() const {
        for (auto *button : buttons()->buttons())
            if (buttons()->buttonRole(button) == QDialogButtonBox::ActionRole)
                return qobject_cast<QPushButton *>(button);
        return nullptr;
    }
    StatusCard *card() const { return m_dialog->findChild<StatusCard *>(); }
    QString status() const { return card()->findChild<QLabel *>()->text(); }
    void respond(ConnectionSecrets::Result result) {
        QVERIFY(bool(m_secretResult));
        auto callback = std::move(m_secretResult);
        callback(result);
    }
    void create(const ConnectionProfile *profile = nullptr) {
        ConnectionDialog::Dependencies dependencies;
        dependencies.readPassword = [this](const QUuid &id, QObject *, auto callback) {
            m_operations << "read";
            m_secretId = id;
            m_secretResult = std::move(callback);
        };
        dependencies.writePassword = [this](const QUuid &id, const QString &password, QObject *, auto callback) {
            m_operations << "write";
            m_secretId = id;
            m_writtenPassword = password;
            m_secretResult = std::move(callback);
        };
        dependencies.removePassword = [this](const QUuid &id, QObject *, auto callback) {
            m_operations << "remove";
            m_secretId = id;
            m_secretResult = std::move(callback);
        };
        dependencies.testConnection = [this](const ConnectionProfile &profile, const QString &password) {
            ++m_checks;
            m_checkedProfile = profile;
            m_checkedPassword = password;
            m_check = std::make_unique<QPromise<ConnectionTestResult>>();
            m_check->start();
            return m_check->future();
        };
        m_dialog = std::make_unique<ConnectionDialog>(m_parent.get(), profile, std::move(dependencies));
        connect(m_dialog.get(), &ConnectionDialog::connectionSubmitted, this,
                [this](const auto &profile) { m_submitted << profile; });
        m_dialog->show();
    }
    void fill() { field("connectionName")->setText("Test connection"); }
    static ConnectionProfile existing(bool withPassword = false) {
        ConnectionProfile profile;
        profile.name = "Existing";
        profile.host = "example.test";
        profile.port = 15432;
        profile.database = "existing db";
        profile.user = "existing user";
        profile.savePassword = withPassword;
        return profile;
    }
    void loadExistingPassword() {
        QTRY_COMPARE(m_operations, QStringList{"read"});
        respond({true, true, "old password", {}});
    }

private slots:
    void initTestCase() { QApplication::setQuitOnLastWindowClosed(false); }
    void init() {
        m_submitted.clear();
        m_operations.clear();
        m_secretResult = {};
        m_checks = 0;
        m_parent = std::make_unique<QWidget>();
        m_parent->resize(800, 700);
        m_parent->show();
    }
    void cleanup() {
        m_dialog.reset();
        m_secretResult = {};
        if (m_check && !m_check->future().isFinished()) m_check->finish();
        m_check.reset();
        m_parent.reset();
    }
    void defaultsAndPasswordMask() {
        create();
        QVERIFY(field("connectionName")->text().isEmpty());
        QCOMPARE(field("connectionHost")->text(), QString("localhost"));
        QCOMPARE(field("connectionPort")->text(), QString("5432"));
        QCOMPARE(field("connectionDatabase")->text(), QString("postgres"));
        QCOMPARE(field("connectionUser")->text(), QString("postgres"));
        QCOMPARE(field("connectionPassword")->echoMode(), QLineEdit::Password);
        QVERIFY(!savePassword()->isChecked());
        QCOMPARE(m_dialog->windowModality(), Qt::WindowModal);
        QVERIFY(m_operations.isEmpty());
    }
    void editLoadsProfileAndPreservesId() {
        const auto original = existing();
        create(&original);
        QCOMPARE(field("connectionName")->text(), original.name);
        QCOMPARE(field("connectionHost")->text(), original.host);
        QCOMPARE(field("connectionPort")->text(), QString::number(original.port));
        QCOMPARE(field("connectionDatabase")->text(), original.database);
        QCOMPARE(field("connectionUser")->text(), original.user);
        field("connectionName")->setText("Edited");
        QTest::mouseClick(save(), Qt::LeftButton);
        QCOMPARE(m_submitted.size(), 1);
        QCOMPARE(m_submitted.first().id, original.id);
        QCOMPARE(m_submitted.first().name, QString("Edited"));
        QCOMPARE(m_submitted.first().host, original.host);
        QVERIFY(m_operations.isEmpty());
    }
    void invalidForm_data() {
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("value");
        for (const auto *name : {"connectionName", "connectionHost", "connectionDatabase", "connectionUser"}) {
            QTest::newRow(name) << QString(name) << "   ";
        }
        QTest::newRow("empty-port") << "connectionPort" << "";
        QTest::newRow("zero-port") << "connectionPort" << "0";
        QTest::newRow("negative-port") << "connectionPort" << "-1";
        QTest::newRow("overflow-port") << "connectionPort" << "65536";
        QTest::newRow("nonnumeric-port") << "connectionPort" << "oops";
    }
    void invalidForm() {
        QFETCH(QString, name);
        QFETCH(QString, value);
        create(); fill();
        field(qPrintable(name))->setText(value);
        QTest::mouseClick(save(), Qt::LeftButton);
        QTest::mouseClick(check(), Qt::LeftButton);
        QVERIFY(m_submitted.isEmpty());
        QCOMPARE(m_checks, 0);
        QVERIFY(m_operations.isEmpty());
        QVERIFY(m_dialog->isVisible());
    }
    void trimsFieldsAndAcceptsOnlyAfterSaveConfirmation() {
        create();
        field("connectionName")->setText("  Test  ");
        field("connectionHost")->setText("  localhost  ");
        field("connectionDatabase")->setText("  my db  ");
        field("connectionUser")->setText("  my user  ");
        field("connectionPort")->setText("65535");
        QSignalSpy accepted(m_dialog.get(), &QDialog::accepted);
        QTest::mouseClick(save(), Qt::LeftButton);
        QCOMPARE(m_submitted.size(), 1);
        const auto &profile = m_submitted.first();
        QCOMPARE(profile.name, QString("Test"));
        QCOMPARE(profile.host, QString("localhost"));
        QCOMPARE(profile.database, QString("my db"));
        QCOMPARE(profile.user, QString("my user"));
        QCOMPARE(profile.port, quint16(65535));
        QVERIFY(!profile.id.isNull());
        QVERIFY(!profile.savePassword);
        QVERIFY(m_dialog->isVisible());
        QCOMPARE(accepted.count(), 0);
        QVERIFY(!save()->isEnabled());
        m_dialog->reject();
        QVERIFY(m_dialog->isVisible());
        m_dialog->completeSave(true);
        QCOMPARE(accepted.count(), 1);
        QVERIFY(!m_dialog->isVisible());
    }
    void enterSubmitsOnce() {
        create(); fill();
        QTest::keyClick(field("connectionName"), Qt::Key_Return);
        QCOMPARE(m_submitted.size(), 1);
        QTest::keyClick(field("connectionName"), Qt::Key_Return);
        QCOMPARE(m_submitted.size(), 1);
    }
    void cancelAndEscapeDoNotSubmit_data() {
        QTest::addColumn<bool>("escape");
        QTest::newRow("button") << false;
        QTest::newRow("escape") << true;
    }
    void cancelAndEscapeDoNotSubmit() {
        QFETCH(bool, escape);
        create(); fill();
        QSignalSpy rejected(m_dialog.get(), &QDialog::rejected);
        if (escape) QTest::keyClick(m_dialog.get(), Qt::Key_Escape);
        else QTest::mouseClick(buttons()->button(QDialogButtonBox::Cancel), Qt::LeftButton);
        QCOMPARE(rejected.count(), 1);
        QVERIFY(m_submitted.isEmpty());
        QVERIFY(m_operations.isEmpty());
    }
    void saveFailureAllowsRetryWithSameId() {
        create(); fill();
        QTest::mouseClick(save(), Qt::LeftButton);
        const auto id = m_submitted.first().id;
        m_dialog->completeSave(false);
        QVERIFY(card()->isVisible());
        QCOMPARE(status(), QString("Could not save connection."));
        QVERIFY(save()->isEnabled());
        field("connectionName")->setText("Retry");
        QVERIFY(!card()->isVisible());
        QTest::mouseClick(save(), Qt::LeftButton);
        QCOMPARE(m_submitted.size(), 2);
        QCOMPARE(m_submitted.last().id, id);
    }
    void checkedEmptyPasswordIsNotStored() {
        create(); fill();
        savePassword()->setChecked(true);
        QTest::mouseClick(save(), Qt::LeftButton);
        QCOMPARE(m_submitted.size(), 1);
        QVERIFY(!m_submitted.first().savePassword);
        QVERIFY(m_operations.isEmpty());
    }
    void writesPasswordBeforeSubmitting() {
        create(); fill();
        field("connectionPassword")->setText("new password ' \\");
        savePassword()->setChecked(true);
        QTest::mouseClick(save(), Qt::LeftButton);
        QCOMPARE(m_operations, QStringList{"write"});
        QCOMPARE(m_writtenPassword, QString("new password ' \\"));
        QVERIFY(m_submitted.isEmpty());
        QVERIFY(!save()->isEnabled());
        respond({true, true, {}, {}});
        QCOMPARE(m_submitted.size(), 1);
        QVERIFY(m_submitted.first().savePassword);
        QCOMPARE(m_submitted.first().id, m_secretId);
    }
    void passwordWriteFailureCanFallBackToUnsavedPassword() {
        create(); fill();
        field("connectionPassword")->setText("secret");
        savePassword()->setChecked(true);
        QTest::mouseClick(save(), Qt::LeftButton);
        respond({false, false, {}, "Keychain unavailable"});
        QVERIFY(m_submitted.isEmpty());
        QVERIFY(card()->isVisible());
        QCOMPARE(status(), QString("Could not save password."));
        QVERIFY(save()->isEnabled());
        savePassword()->setChecked(false);
        QTest::mouseClick(save(), Qt::LeftButton);
        QCOMPARE(m_submitted.size(), 1);
        QVERIFY(!m_submitted.first().savePassword);
        QCOMPARE(m_operations, QStringList{"write"});
    }
    void loadsExistingPasswordAndAvoidsUnnecessaryWrite() {
        auto original = existing(true);
        create(&original);
        loadExistingPassword();
        QCOMPARE(m_secretId, original.id);
        QCOMPARE(field("connectionPassword")->text(), QString("old password"));
        QVERIFY(savePassword()->isChecked());
        QTest::mouseClick(save(), Qt::LeftButton);
        QCOMPARE(m_submitted.size(), 1);
        QCOMPARE(m_operations, QStringList{"read"});
    }
    void missingSavedPasswordClearsCheckbox() {
        auto original = existing(true);
        create(&original);
        QTRY_COMPARE(m_operations, QStringList{"read"});
        respond({true, false, {}, {}});
        QVERIFY(!savePassword()->isChecked());
        QVERIFY(field("connectionPassword")->text().isEmpty());
        QVERIFY(!card()->isVisible());
    }
    void failedReadRetriesBeforeSaving() {
        auto original = existing(true);
        create(&original);
        QTRY_COMPARE(m_operations, QStringList{"read"});
        respond({false, false, {}, "Denied"});
        QCOMPARE(status(), QString("Could not load password."));
        QTest::mouseClick(save(), Qt::LeftButton);
        QCOMPARE(m_operations, (QStringList{"read", "read"}));
        QVERIFY(m_submitted.isEmpty());
        respond({true, true, "old password", {}});
        QTest::mouseClick(save(), Qt::LeftButton);
        QCOMPARE(m_submitted.size(), 1);
        QCOMPARE(m_operations.size(), 2);
    }
    void disablingPasswordRemovesExistingSecret() {
        auto original = existing(true);
        create(&original);
        loadExistingPassword();
        savePassword()->setChecked(false);
        QTest::mouseClick(save(), Qt::LeftButton);
        QCOMPARE(m_operations.last(), QString("remove"));
        QVERIFY(m_submitted.isEmpty());
        respond({true, false, {}, {}});
        QCOMPARE(m_submitted.size(), 1);
        QVERIFY(!m_submitted.first().savePassword);
    }
    void saveFailureRollsBackSecret_data() {
        QTest::addColumn<bool>("hadPassword");
        QTest::addColumn<bool>("rollbackSuccess");
        QTest::newRow("remove-new-secret") << false << true;
        QTest::newRow("restore-old-secret") << true << true;
        QTest::newRow("rollback-failure") << true << false;
    }
    void saveFailureRollsBackSecret() {
        QFETCH(bool, hadPassword);
        QFETCH(bool, rollbackSuccess);
        auto original = existing(hadPassword);
        create(&original);
        if (hadPassword) loadExistingPassword();
        field("connectionPassword")->setText("replacement");
        savePassword()->setChecked(true);
        QTest::mouseClick(save(), Qt::LeftButton);
        QCOMPARE(m_operations.last(), QString("write"));
        respond({true, true, {}, {}});
        QCOMPARE(m_submitted.size(), 1);
        m_dialog->completeSave(false);
        QCOMPARE(m_operations.last(), hadPassword ? QString("write") : QString("remove"));
        if (hadPassword) QCOMPARE(m_writtenPassword, QString("old password"));
        QCOMPARE(m_secretId, original.id);
        respond({rollbackSuccess, hadPassword, {}, "Restore failed"});
        QVERIFY(save()->isEnabled());
        QCOMPARE(status(), rollbackSuccess ? QString("Could not save connection.")
                                          : QString("Could not restore password."));
        QVERIFY(m_dialog->isVisible());
    }
    void testConnectionResults_data() {
        QTest::addColumn<bool>("success");
        QTest::newRow("success") << true;
        QTest::newRow("failure") << false;
    }
    void testConnectionResults() {
        QFETCH(bool, success);
        create(); fill();
        field("connectionPassword")->setText("test password");
        QTest::mouseClick(check(), Qt::LeftButton);
        QCOMPARE(m_checks, 1);
        QCOMPARE(m_checkedProfile.name, QString("Test connection"));
        QCOMPARE(m_checkedPassword, QString("test password"));
        QVERIFY(!save()->isEnabled());
        QVERIFY(!check()->isEnabled());
        QVERIFY(m_submitted.isEmpty());
        QCOMPARE(status(), QString("Testing connection…"));
        m_check->addResult({success, "Connection refused"});
        m_check->finish();
        QTRY_VERIFY(check()->isEnabled());
        QVERIFY(save()->isEnabled());
        QCOMPARE(status(), success ? QString("Connection successful.") : QString("Could not connect."));
        QVERIFY(m_submitted.isEmpty());
        QVERIFY(m_operations.isEmpty());
    }
    void editedFormIgnoresOldTestResult() {
        create(); fill();
        QTest::mouseClick(check(), Qt::LeftButton);
        field("connectionHost")->setText("changed.example.test");
        QVERIFY(!card()->isVisible());
        m_check->addResult({false, "Old error"});
        m_check->finish();
        QTRY_VERIFY(check()->isEnabled());
        QVERIFY(!card()->isVisible());
    }
    void canCloseWhileTestingAndFinishSafely() {
        create(); fill();
        QTest::mouseClick(check(), Qt::LeftButton);
        QTest::keyClick(m_dialog.get(), Qt::Key_Escape);
        QVERIFY(!m_dialog->isVisible());
        m_dialog.reset();
        m_check->addResult({true, {}});
        m_check->finish();
        QCoreApplication::processEvents();
        QVERIFY(m_submitted.isEmpty());
    }
};

QTEST_MAIN(ConnectionDialogTest)
#include "connectiondialogtest.moc"
