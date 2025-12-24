#include "ApiClient.h"

#include <QDateTime>
#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

namespace {
const char *kEndpointUrl =
    "https://script.google.com/macros/s/AKfycbyyHjCS0qBVtI4jDD9HiqT2kRnMV6U0pOQLUT68kRMlp2i7A1KAqtu1CwFT1DGiq58W/exec";

struct ReplyPayload {
    int statusCode = 0;
    QString contentType;
    QByteArray body;
    QString error;
};

ReplyPayload waitForReply(QNetworkReply *reply) {
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    ReplyPayload payload;
    payload.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    payload.contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString().toLower();
    payload.body = reply->readAll();
    if (reply->error() != QNetworkReply::NoError) {
        payload.error = reply->errorString();
    }
    reply->deleteLater();
    return payload;
}
} // namespace

ApiClient::ApiClient(QObject *parent) : QObject(parent) {}

QString ApiClient::endpointUrl() {
    return QString::fromUtf8(kEndpointUrl);
}

ApiClient::Result ApiClient::sendPost(const QJsonObject &payload) {
    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl(endpointUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = manager.post(request, QJsonDocument(payload).toJson());
    ReplyPayload payloadData = waitForReply(reply);

    Result result;
    if (!payloadData.error.isEmpty()) {
        result.ok = false;
        result.message = QString::fromUtf8("❌ 連線失敗：%1").arg(payloadData.error);
        return result;
    }

    if (payloadData.statusCode != 200) {
        result.ok = false;
        result.message = QString::fromUtf8("❌ HTTP %1\n%2")
                             .arg(payloadData.statusCode)
                             .arg(QString::fromUtf8(payloadData.body.left(200)));
        return result;
    }

    if (!payloadData.contentType.contains("application/json")) {
        result.ok = true;
        result.message = QString::fromUtf8("✅ 新增成功（回應非JSON）");
        return result;
    }

    QJsonDocument doc = QJsonDocument::fromJson(payloadData.body);
    if (!doc.isObject()) {
        result.ok = true;
        result.message = QString::fromUtf8("✅ 新增成功（回應非JSON）");
        return result;
    }

    QJsonObject obj = doc.object();
    if (obj.value("ok").isBool() && !obj.value("ok").toBool()) {
        result.ok = false;
        result.message = QString::fromUtf8("❌ 新增失敗：%1").arg(obj.value("error").toString("未知錯誤"));
        return result;
    }

    result.ok = true;
    result.message = QString::fromUtf8("✅ 新增成功");
    return result;
}

ApiClient::Result ApiClient::sendGet(const QUrl &url) {
    QNetworkAccessManager manager;
    QNetworkRequest request(url);

    QNetworkReply *reply = manager.get(request);
    ReplyPayload payloadData = waitForReply(reply);

    Result result;
    if (!payloadData.error.isEmpty()) {
        result.ok = false;
        result.message = QString::fromUtf8("❌ 連線失敗：%1").arg(payloadData.error);
        return result;
    }

    if (payloadData.statusCode != 200) {
        result.ok = false;
        result.message = QString::fromUtf8("❌ HTTP %1\n%2")
                             .arg(payloadData.statusCode)
                             .arg(QString::fromUtf8(payloadData.body.left(200)));
        return result;
    }

    if (!payloadData.contentType.contains("application/json")) {
        result.ok = false;
        result.message = QString::fromUtf8("❌ 回應非JSON（可能權限/網址錯）\n%1")
                             .arg(QString::fromUtf8(payloadData.body.left(200)));
        return result;
    }

    QJsonDocument doc = QJsonDocument::fromJson(payloadData.body);
    if (!doc.isObject()) {
        result.ok = false;
        result.message = QString::fromUtf8("❌ 回應格式錯誤");
        return result;
    }

    QJsonObject obj = doc.object();
    if (obj.value("ok").isBool() && !obj.value("ok").toBool()) {
        result.ok = false;
        result.message = QString::fromUtf8("❌ 查詢失敗：%1").arg(obj.value("error").toString("未知錯誤"));
        return result;
    }

    result.ok = true;
    result.rows = obj.value("rows").toArray();
    return result;
}

ApiClient::Result ApiClient::postRecord(const QJsonObject &data) {
    QJsonObject payload;
    payload.insert("type", "customer_service");
    payload.insert("timestamp", static_cast<qint64>(QDateTime::currentSecsSinceEpoch()));
    payload.insert("data", data);
    return sendPost(payload);
}

ApiClient::Result ApiClient::getRecords(const QString &phone, bool onlyWater) {
    QUrl url(endpointUrl());
    QUrlQuery query;
    query.addQueryItem("phone", phone);
    if (onlyWater) {
        query.addQueryItem("only_water", "1");
    }
    url.setQuery(query);
    return sendGet(url);
}

ApiClient::Result ApiClient::fetchRaw(const QString &phone) {
    QUrl url(endpointUrl());
    QUrlQuery query;
    query.addQueryItem("phone", phone);
    url.setQuery(query);
    return sendGet(url);
}
