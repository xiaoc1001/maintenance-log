#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>

class ApiClient : public QObject {
    Q_OBJECT

public:
    explicit ApiClient(QObject *parent = nullptr);

    struct Result {
        bool ok = false;
        QString message;
        QJsonArray rows;
    };

    Result postRecord(const QJsonObject &data);
    Result getRecords(const QString &phone, bool onlyWater);
    Result fetchRaw(const QString &phone);

private:
    QString endpointUrl();
    Result sendPost(const QJsonObject &payload);
    Result sendGet(const QUrl &url);
};
