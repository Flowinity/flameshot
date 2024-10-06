//
// Created by troplo on 6/10/24.
//

#include "EndpointsJSON.h"
#include "confighandler.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QtGlobal>
#include "abstractlogger.h"

EndpointsJSON::EndpointsJSON(QObject* parent)
        : QObject(parent)
        , m_NetworkAM(new QNetworkAccessManager(this))
{}

void EndpointsJSON::getAPIFromEndpoints(bool refresh)
{
    if (!refresh) {
        const QString endpoint = ConfigHandler().serverAPIEndpoint();
        if (!endpoint.isEmpty()) {
            emit endpointOk(endpoint);
            return;
        }
    }

    const QString tpuUrl = ConfigHandler().serverTPU();
    const QString endpointPrimary = tpuUrl + "/endpoints.json";
    const QString endpointFallback = tpuUrl + "/endpoints.local.json";

    QNetworkRequest requestPrimary;
    requestPrimary.setUrl(QUrl(endpointPrimary));
    requestPrimary.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    AbstractLogger::info() << "Requesting URL: " << endpointPrimary;

    QNetworkReply* replyPrimary = m_NetworkAM->get(requestPrimary);

    connect(replyPrimary, &QNetworkReply::finished, this, [this, replyPrimary, endpointFallback]() {
        AbstractLogger::info() << "Primary endpoint request finished";
        if (replyPrimary->error() == QNetworkReply::NoError) {
            handleAPIEndpointResponse(replyPrimary);
        } else {
            AbstractLogger::error() << "Primary endpoint request failed: " << replyPrimary->errorString();

            QNetworkRequest requestFallback;
            requestFallback.setUrl(QUrl(endpointFallback));
            requestFallback.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            AbstractLogger::info() << "Requesting URL: " << endpointFallback;

            QNetworkReply* replyFallback = m_NetworkAM->get(requestFallback);

            connect(replyFallback, &QNetworkReply::finished, this, [this, replyFallback]() {
                AbstractLogger::info() << "Fallback endpoint request finished";
                if (replyFallback->error() == QNetworkReply::NoError) {
                    handleAPIEndpointResponse(replyFallback);
                } else {
                    AbstractLogger::error() << "Fallback endpoint request failed: " << replyFallback->errorString();
                    emit error(replyFallback->errorString());
                }
                replyFallback->deleteLater();
            });
        }
        replyPrimary->deleteLater();
    });
}

void EndpointsJSON::handleAPIEndpointResponse(QNetworkReply* reply)
{
    QJsonParseError jsonError;
    QJsonDocument const json = QJsonDocument::fromJson(reply->readAll(), &jsonError);
    if (jsonError.error != QJsonParseError::NoError) {
        emit error(jsonError.errorString());
        return;
    }

    QString response = json.object().value("api").toArray().at(0).toObject().value("url").toString();
    if (!response.isEmpty()) {
        emit endpointOk(response);
    } else {
        emit error("[Flowinity/Endpoints.json] Invalid JSON structure: missing 'api[0].url'");
    }
}

void EndpointsJSON::getAPIEndpoint(bool refresh) {
    getAPIFromEndpoints(refresh);
    connect(this, &EndpointsJSON::error, this, [this](QString error) {
        disconnect(this, &EndpointsJSON::error, nullptr, nullptr);
        AbstractLogger::error() << "Error getting API endpoint: " << error;

        const QString fallbackEndpoint = ConfigHandler().serverTPU() + "/api/v3";
        ConfigHandler().setServerAPIEndpoint(fallbackEndpoint);
        ConfigHandler().setServerSupportsEndpoints(false);
        emit endpointOk(fallbackEndpoint);
    });

    connect(this, &EndpointsJSON::endpointOk, this, [this](QString endpoint) {
        AbstractLogger::info() << "API endpoint: " << endpoint;
        disconnect(this, &EndpointsJSON::endpointOk, nullptr, nullptr);
        ConfigHandler().setServerAPIEndpoint(endpoint);
        ConfigHandler().setServerSupportsEndpoints(true);
        emit endpointOk(endpoint);
    });
}