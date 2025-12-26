#include "ApiClient.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

namespace {
const char *kEndpointUrl =
    "https://script.google.com/macros/s/AKfycbyyHjCS0qBVtI4jDD9HiqT2kRnMV6U0pOQLUT68kRMlp2i7A1KAqtu1CwFT1DGiq58W/exec";
} // namespace

ApiClient::ApiClient(QObject *parent) : QObject(parent) {}

QString ApiClient::endpointUrl() {
    return QString::fromUtf8(kEndpointUrl);
}

void ApiClient::sendPostAsync(const QJsonObject &payload, ResultHandler handler) {
    QNetworkRequest request(QUrl(endpointUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = manager.post(request, QJsonDocument(payload).toJson());
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, handler]() {
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString().toLower();
        const QByteArray body = reply->readAll();
        const QString error = reply->error() == QNetworkReply::NoError ? QString() : reply->errorString();
        reply->deleteLater();

        if (!error.isEmpty()) {
            handler(buildErrorResult(QString::fromUtf8("❌ 連線失敗：%1").arg(error)));
            return;
        }

        if (statusCode != 200) {
            handler(buildErrorResult(QString::fromUtf8("❌ HTTP %1\n%2")
                                         .arg(statusCode)
                                         .arg(QString::fromUtf8(body.left(200)))));
            return;
        }

        if (!contentType.contains("application/json")) {
            Result result;
            result.ok = true;
            result.message = QString::fromUtf8("✅ 新增成功（回應非JSON）");
            handler(result);
            return;
        }

        Result result = parseJsonResult(body, false, QString::fromUtf8("新增"));
        handler(result);
    });
}

void ApiClient::sendGetAsync(const QUrl &url, ResultHandler handler) {
    QNetworkRequest request(url);

    QNetworkReply *reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, handler]() {
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString().toLower();
        const QByteArray body = reply->readAll();
        const QString error = reply->error() == QNetworkReply::NoError ? QString() : reply->errorString();
        reply->deleteLater();

        if (!error.isEmpty()) {
            handler(buildErrorResult(QString::fromUtf8("❌ 連線失敗：%1").arg(error)));
            return;
        }

        if (statusCode != 200) {
            handler(buildErrorResult(QString::fromUtf8("❌ HTTP %1\n%2")
                                         .arg(statusCode)
                                         .arg(QString::fromUtf8(body.left(200)))));
            return;
        }

        if (!contentType.contains("application/json")) {
            handler(buildErrorResult(QString::fromUtf8("❌ 回應非JSON（可能權限/網址錯）\n%1")
                                         .arg(QString::fromUtf8(body.left(200)))));
            return;
        }

        handler(parseJsonResult(body, true, QString::fromUtf8("查詢")));
    });
}

void ApiClient::postRecordAsync(const QJsonObject &data, ResultHandler handler) {
    QJsonObject payload;
    payload.insert("type", "customer_service");
    payload.insert("timestamp", static_cast<qint64>(QDateTime::currentSecsSinceEpoch()));
    payload.insert("data", data);
    sendPostAsync(payload, handler);
}

void ApiClient::getRecordsAsync(const QString &phone, bool onlyWater, ResultHandler handler) {
    QUrl url(endpointUrl());
    QUrlQuery query;
    query.addQueryItem("phone", phone);
    if (onlyWater) {
        query.addQueryItem("only_water", "1");
    }
    url.setQuery(query);
    sendGetAsync(url, handler);
}

void ApiClient::fetchRawAsync(const QString &phone, ResultHandler handler) {
    QUrl url(endpointUrl());
    QUrlQuery query;
    query.addQueryItem("phone", phone);
    url.setQuery(query);
    sendGetAsync(url, handler);
}

ApiClient::Result ApiClient::buildErrorResult(const QString &message) const {
    Result result;
    result.ok = false;
    result.message = message;
    return result;
}

ApiClient::Result ApiClient::parseJsonResult(const QByteArray &body, bool expectRows, const QString &errorPrefix) const {
    QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        return buildErrorResult(QString::fromUtf8("❌ 回應格式錯誤"));
    }

    QJsonObject obj = doc.object();
    if (obj.value("ok").isBool() && !obj.value("ok").toBool()) {
        return buildErrorResult(QString::fromUtf8("❌ %1失敗：%2").arg(errorPrefix, obj.value("error").toString("未知錯誤")));
    }

    Result result;
    result.ok = true;
    if (expectRows) {
        result.rows = obj.value("rows").toArray();
    } else {
        result.message = QString::fromUtf8("✅ 新增成功");
    }
    return result;
}
