#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

#include <functional>

class ApiClient : public QObject {
    Q_OBJECT

public:
    explicit ApiClient(QObject *parent = nullptr);

    struct Result {
        bool ok = false;
        QString message;
        QJsonArray rows;
    };

    using ResultHandler = std::function<void(const Result &)>;

    void postRecordAsync(const QJsonObject &data, ResultHandler handler);
    void getRecordsAsync(const QString &phone, bool onlyWater, ResultHandler handler);
    void fetchRawAsync(const QString &phone, ResultHandler handler);

private:
    QString endpointUrl();
    void sendPostAsync(const QJsonObject &payload, ResultHandler handler);
    void sendGetAsync(const QUrl &url, ResultHandler handler);
    Result buildErrorResult(const QString &message) const;
    Result parseJsonResult(const QByteArray &body, bool expectRows, const QString &errorPrefix) const;

    QNetworkAccessManager manager;
};
