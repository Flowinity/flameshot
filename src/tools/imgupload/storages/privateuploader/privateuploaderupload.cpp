// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2023 Troplo & Contributors

#include "privateuploaderupload.h"
#include "flowinity/EndpointsJSON.h"
#include "src/utils/confighandler.h"
#include "src/utils/filenamehandler.h"
#include <QDesktopServices>
#include <QEventLoop>
#include <QFile>
#include <QHttpPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QShortcut>
#include <QUrlQuery>
#include <QtGlobal>
#include <iostream>
#include "abstractlogger.h"
PrivateUploaderUpload::PrivateUploaderUpload(QObject* parent)
  : QObject(parent)
  , m_NetworkAM(
      new QNetworkAccessManager(this))
{}

QByteArray const boundary = "BoUnDaRy";

void PrivateUploaderUpload::uploadToServer(const QByteArray& postData, const QString& url, const QString& token)
{

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("Authorization", token.toUtf8());
    request.setRawHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

    emit uploadProgress(0);

    QNetworkReply* reply = m_NetworkAM->post(request, postData);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            emit uploadOk(reply);
        } else {
            EndpointsJSON* endpoints = new EndpointsJSON(this);
            endpoints->getAPIFromEndpoints(true);
            emit uploadError(reply);
        }

        reply->deleteLater();
    });

    connect(reply, &QNetworkReply::uploadProgress, [this, reply](qint64 bytesSent, qint64 bytesTotal) {
        if (bytesTotal == 0)
            return;
        emit uploadProgress(bytesSent * 100 / bytesTotal);
    });
}

void PrivateUploaderUpload::uploadBytes(const QByteArray& byteArray, const QString& fileName, const QString& fileType)
{
    QByteArray postData;

    // Append the beginning of the form-data
    postData.append("--" + boundary + "\r\n");
    postData.append(R"(Content-Disposition: form-data; name="attachment"; filename=")" + fileName.toUtf8() + "\"\r\n");
    postData.append("Content-Type: " + fileType.toUtf8() + "\r\n");
    postData.append("\r\n");

    // Append the image data
    postData.append(byteArray);
    postData.append("\r\n");

    // Append the end of the form-data
    postData.append("--" + boundary + "--\r\n");

    QString url = QStringLiteral("%1/gallery").arg(ConfigHandler().serverAPIEndpoint());
    QString token = QStringLiteral("%1").arg(ConfigHandler().uploadTokenTPU());

    uploadToServer(postData, url, token);
}

void PrivateUploaderUpload::uploadFile(const QString& filePath, const QString& fileName, const QString& fileType)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        // Handle error
        return;
    }

    QString url = QStringLiteral("%1/gallery").arg(ConfigHandler().serverAPIEndpoint());
    QString token = QStringLiteral("%1").arg(ConfigHandler().uploadTokenTPU());

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("Authorization", token.toUtf8());
    request.setRawHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

    QByteArray headerData = QString("--" + boundary + "\r\n" +
                                    "Content-Disposition: form-data; name=\"attachment\"; filename=\"" +
                                    fileName.toUtf8() + "\"\r\n" +
                                    "Content-Type: " + fileType.toUtf8() + "\r\n" +
                                    "\r\n").toUtf8();

    QByteArray footerData = QString("\r\n--" + boundary + "--\r\n").toUtf8();

    qint64 fileSize = file.size();
    qint64 bytesUploaded = 0;
    qint64 chunkSize = 4096 * 4096;

    QNetworkAccessManager manager;
    QNetworkReply* reply = nullptr;

    while (bytesUploaded < fileSize) {
        QByteArray postData;
        postData.append(headerData);

        qint64 bytesToRead = qMin(chunkSize, fileSize - bytesUploaded);
        QByteArray chunk = file.read(bytesToRead);
        postData.append(chunk);

        if (bytesUploaded + bytesToRead == fileSize) {
            postData.append(footerData);
        }

        reply = manager.post(request, postData);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() != QNetworkReply::NoError) {
            // Handle error
            qDebug() << "Error occurred during upload:" << reply->errorString();
            break;
        } else {
            qDebug() << "Upload successful!";
        }

        bytesUploaded += bytesToRead;
    }

    if (reply) {
        reply->deleteLater();
    }
}
