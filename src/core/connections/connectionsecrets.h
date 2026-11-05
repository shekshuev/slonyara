#pragma once

#include <QString>
#include <QUuid>
#include <functional>

class QObject;

namespace ConnectionSecrets {
struct Result {
    bool success = false;
    bool found = false;
    QString password;
    QString error;
};
using Callback = std::function<void(const Result &)>;

void read(const QUuid &id, QObject *context, Callback callback);
void write(const QUuid &id, const QString &password, QObject *context, Callback callback);
void remove(const QUuid &id, QObject *context, Callback callback);
}
