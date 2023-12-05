// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "privateuploaderupload.h"
#include "src/utils/confighandler.h"
#include "src/utils/filenamehandler.h"
#include <QDesktopServices>
#include <QEventLoop>
#include <QHttpPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QShortcut>
#include <QUrlQuery>
#include <iostream>

PrivateUploaderUpload::PrivateUploaderUpload(QObject* parent)
  : QObject(parent)
  , m_NetworkAM(
      new QNetworkAccessManager(this)) // Initialize member in the constructor
{}

void PrivateUploaderUpload::uploadBytes(const QByteArray& byteArray,
                                        const QString& fileName,
                                        const QString& fileType)
{
    QByteArray const boundary = "BoUnDaRy";
    QByteArray postData;

    // Append the beginning of the form-data
    postData.append("--" + boundary + "\r\n");
    postData.append(
      R"(Content-Disposition: form-data; name="attachment"; filename=")" +
      fileName.toUtf8() + "\"\r\n");
    postData.append("Content-Type: " + fileType.toUtf8() + "\r\n");
    postData.append("\r\n");

    // Append the image data
    postData.append(byteArray);
    postData.append("\r\n");

    // Append the end of the form-data
    postData.append("--" + boundary + "--\r\n");

    QUrl const url(
      QStringLiteral("%1/api/v3/gallery").arg(ConfigHandler().serverTPU()));
    QNetworkRequest request(url);
    request.setRawHeader(
      "Authorization",
      QStringLiteral("%1").arg(ConfigHandler().uploadTokenTPU()).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "multipart/form-data; boundary=" + boundary);

    emit uploadProgress(0);

    QNetworkReply* reply = m_NetworkAM->post(request, postData);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            emit uploadOk(reply);
        } else {
            emit uploadError(reply);
        }

        reply->deleteLater();
    });

    connect(reply,
            &QNetworkReply::uploadProgress,
            [this, reply](qint64 bytesSent, qint64 bytesTotal) {
                // Will crash if bytesTotal is 0 (division by zero)
                if (bytesTotal == 0)
                    return;
                emit uploadProgress(bytesSent * 100 / bytesTotal);
            });
}