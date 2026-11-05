#include "core/connections/connectionstore.h"

#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <memory>

class ConnectionStoreTest final : public QObject
{
    Q_OBJECT
private:
    std::unique_ptr<QTemporaryDir> m_directory;

    static ConnectionProfile profile(const QString &name) {
        ConnectionProfile result;
        result.name = name;
        result.host = "db.example.test";
        result.port = 15432;
        result.database = "database with spaces";
        result.user = "user'\\name";
        result.savePassword = true;
        return result;
    }
    static void compareProfiles(const ConnectionProfile &actual, const ConnectionProfile &expected) {
        QCOMPARE(actual.id, expected.id);
        QCOMPARE(actual.name, expected.name);
        QCOMPARE(actual.host, expected.host);
        QCOMPARE(actual.port, expected.port);
        QCOMPARE(actual.database, expected.database);
        QCOMPARE(actual.user, expected.user);
        QCOMPARE(actual.savePassword, expected.savePassword);
    }
    static void writeSettings(const QVariantMap &values) {
        QSettings settings;
        settings.beginWriteArray("connections", 1);
        settings.setArrayIndex(0);
        for (auto it = values.cbegin(); it != values.cend(); ++it)
            settings.setValue(it.key(), it.value());
        settings.endArray();
        settings.sync();
        QCOMPARE(settings.status(), QSettings::NoError);
    }

private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName("SlonyaraTests");
        QCoreApplication::setApplicationName("ConnectionStoreTest");
        QSettings::setDefaultFormat(QSettings::IniFormat);
    }
    void init() {
        m_directory = std::make_unique<QTemporaryDir>();
        QVERIFY(m_directory->isValid());
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_directory->path());
        QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, m_directory->path());
    }
    void startsEmptyWithoutSettingsFile() {
        QVERIFY(!QFile::exists(QSettings().fileName()));
        ConnectionStore store;
        QVERIFY(store.connections().isEmpty());
    }
    void savesAndLoadsAllFieldsInOrder() {
        auto first = profile(QString::fromUtf8("Локальная БД 🐘"));
        auto second = profile("Second server");
        second.host = "::1";
        second.port = 5432;
        second.database = "other";
        second.user = "readonly";
        second.savePassword = false;
        ConnectionStore store;
        store.addConnection(first);
        store.addConnection(second);
        QVERIFY(store.save());
        ConnectionStore loaded;
        QCOMPARE(loaded.connections().size(), 2);
        compareProfiles(loaded.connections().at(0), first);
        compareProfiles(loaded.connections().at(1), second);
    }
    void changesAreNotPersistedUntilSave() {
        ConnectionStore store;
        store.addConnection(profile("Unsaved"));
        QCOMPARE(store.connections().size(), 1);
        QVERIFY(ConnectionStore().connections().isEmpty());
        QVERIFY(store.save());
        QCOMPARE(ConnectionStore().connections().size(), 1);
    }
    void updatesByIdAndPreservesOtherProfiles() {
        auto first = profile("First");
        auto second = profile("Second");
        ConnectionStore store;
        store.addConnection(first);
        store.addConnection(second);
        QVERIFY(store.save());
        auto updated = profile("Updated");
        updated.id = first.id;
        updated.host = "new.example.test";
        updated.port = 6543;
        updated.database = "new database";
        updated.user = "new user";
        updated.savePassword = false;
        QVERIFY(store.updateConnection(updated));
        QVERIFY(store.save());
        ConnectionStore loaded;
        QCOMPARE(loaded.connections().size(), 2);
        compareProfiles(loaded.connections().at(0), updated);
        compareProfiles(loaded.connections().at(1), second);
    }
    void unknownIdDoesNotModifyStore() {
        auto original = profile("Original");
        ConnectionStore store;
        store.addConnection(original);
        QVERIFY(!store.updateConnection(profile("Unknown")));
        QCOMPARE(store.connections().size(), 1);
        compareProfiles(store.connections().first(), original);
    }
    void removalPersistsWithoutResurrectingOldEntries() {
        auto first = profile("First");
        auto second = profile("Second");
        auto third = profile("Third");
        ConnectionStore store;
        store.addConnection(first);
        store.addConnection(second);
        store.addConnection(third);
        QVERIFY(store.save());
        store.removeConnectionAt(1);
        QVERIFY(store.save());
        ConnectionStore loaded;
        QCOMPARE(loaded.connections().size(), 2);
        compareProfiles(loaded.connections().at(0), first);
        compareProfiles(loaded.connections().at(1), third);
        loaded.removeConnectionAt(1);
        loaded.removeConnectionAt(0);
        QVERIFY(loaded.save());
        QVERIFY(ConnectionStore().connections().isEmpty());
    }
    void invalidRemovalIndex_data() {
        QTest::addColumn<int>("index");
        QTest::newRow("negative") << -1;
        QTest::newRow("past-last") << 1;
        QTest::newRow("large") << 100;
    }
    void invalidRemovalIndex() {
        QFETCH(int, index);
        auto original = profile("Original");
        ConnectionStore store;
        store.removeConnectionAt(index);
        store.addConnection(original);
        store.removeConnectionAt(index);
        QCOMPARE(store.connections().size(), 1);
        compareProfiles(store.connections().first(), original);
    }
    void portValidation_data() {
        QTest::addColumn<QVariant>("stored");
        QTest::addColumn<quint16>("expected");
        QTest::newRow("missing") << QVariant{} << quint16(5432);
        QTest::newRow("text") << QVariant("invalid") << quint16(5432);
        QTest::newRow("zero") << QVariant(0) << quint16(5432);
        QTest::newRow("negative") << QVariant(-1) << quint16(5432);
        QTest::newRow("too-large") << QVariant(65536) << quint16(5432);
        QTest::newRow("minimum") << QVariant(1) << quint16(1);
        QTest::newRow("maximum") << QVariant(65535) << quint16(65535);
        QTest::newRow("numeric-string") << QVariant("15432") << quint16(15432);
    }
    void portValidation() {
        QFETCH(QVariant, stored);
        QFETCH(quint16, expected);
        QVariantMap values{{"name", "Port test"}};
        if (stored.isValid()) values.insert("port", stored);
        writeSettings(values);
        ConnectionStore store;
        QCOMPARE(store.connections().size(), 1);
        QCOMPARE(store.connections().first().port, expected);
    }
    void missingFieldsUseDefaults() {
        writeSettings({});
        ConnectionStore store;
        QCOMPARE(store.connections().size(), 1);
        const auto &loaded = store.connections().first();
        QVERIFY(!loaded.id.isNull());
        QVERIFY(loaded.name.isEmpty());
        QVERIFY(loaded.host.isEmpty());
        QVERIFY(loaded.database.isEmpty());
        QVERIFY(loaded.user.isEmpty());
        QCOMPARE(loaded.port, quint16(5432));
        QVERIFY(!loaded.savePassword);
    }
    void invalidIdIsReplacedAndPersisted_data() {
        QTest::addColumn<QVariant>("storedId");
        QTest::newRow("missing") << QVariant{};
        QTest::newRow("malformed") << QVariant("not-a-uuid");
        QTest::newRow("null") << QVariant(QUuid{});
    }
    void invalidIdIsReplacedAndPersisted() {
        QFETCH(QVariant, storedId);
        QVariantMap values;
        if (storedId.isValid()) values.insert("id", storedId);
        writeSettings(values);
        ConnectionStore store;
        QCOMPARE(store.connections().size(), 1);
        const auto assignedId = store.connections().first().id;
        QVERIFY(!assignedId.isNull());
        QVERIFY(store.save());
        QCOMPARE(ConnectionStore().connections().first().id, assignedId);
    }
    void saveReturnsFalseWhenSettingsPathIsBlocked()
    {
        const QString blockedPath = m_directory->filePath("blocked");
        QFile blocker(blockedPath);
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        blocker.close();
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, blockedPath);
        ConnectionStore store;
        const auto original = profile("Cannot save");
        store.addConnection(original);
        QVERIFY(!store.save());
        QCOMPARE(store.connections().size(), 1);
        compareProfiles(store.connections().first(), original);
    }
};

QTEST_GUILESS_MAIN(ConnectionStoreTest)
#include "connectionstoretest.moc"
