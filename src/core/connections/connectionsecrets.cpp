#include "connectionsecrets.h"
#include <qtkeychain/keychain.h>

namespace {
const QString Service = QStringLiteral("Slonyara");

void start(QKeychain::Job *job, const QUuid &id, QObject *context,
           ConnectionSecrets::Callback callback)
{
    job->setKey(QStringLiteral("connections/%1/password")
                    .arg(id.toString(QUuid::WithoutBraces)));
    job->setInsecureFallback(false);
    QObject::connect(job, &QKeychain::Job::finished, context,
                     [callback = std::move(callback)](QKeychain::Job *finished) {
        ConnectionSecrets::Result result;
        result.found = finished->error() == QKeychain::NoError;
        result.success = result.found || finished->error() == QKeychain::EntryNotFound;
        if (!result.success)
            result.error = finished->errorString();
        if (result.found) {
            if (auto *read = qobject_cast<QKeychain::ReadPasswordJob *>(finished))
                result.password = read->textData();
        }
        callback(result);
    });
    job->start();
}
}

void ConnectionSecrets::read(const QUuid &id, QObject *context, Callback callback)
{
    start(new QKeychain::ReadPasswordJob(Service, context), id, context, std::move(callback));
}

void ConnectionSecrets::write(const QUuid &id, const QString &password,
                              QObject *context, Callback callback)
{
    auto *job = new QKeychain::WritePasswordJob(Service, context);
    job->setTextData(password);
    start(job, id, context, std::move(callback));
}

void ConnectionSecrets::remove(const QUuid &id, QObject *context, Callback callback)
{
    start(new QKeychain::DeletePasswordJob(Service, context), id, context, std::move(callback));
}
