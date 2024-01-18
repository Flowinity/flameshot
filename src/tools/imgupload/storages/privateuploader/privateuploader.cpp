// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2023 Troplo & Contributors

#include "privateuploader.h"
#include "privateuploaderupload.h"
#include "src/utils/confighandler.h"
#include "src/utils/filenamehandler.h"
#include "src/utils/history.h"
#include "src/widgets/loadspinner.h"
#include "src/widgets/notificationwidget.h"
#include <QBuffer>
#include <QDesktopServices>
#include <QEventLoop>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QShortcut>
#include <QUrlQuery>
#include <iostream>

PrivateUploader::PrivateUploader(const QPixmap& capture, QWidget* parent)
  : ImgUploaderBase(capture, parent)
{
    m_NetworkAM = new QNetworkAccessManager(this);
    connect(m_NetworkAM,
            &QNetworkAccessManager::finished,
            this,
            &PrivateUploader::handleReply);
}

void PrivateUploader::handleReply(QNetworkReply* reply)
{
    m_currentImageName.clear();
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
        QJsonObject json = response.object();
        setImageURL(json[QStringLiteral("url")].toString());
        QJsonObject upload = json[QStringLiteral("upload")].toObject();

        // save history
        m_currentImageName = imageURL().toString();
        int lastSlash = m_currentImageName.lastIndexOf("/");
        if (lastSlash >= 0) {
            m_currentImageName = m_currentImageName.mid(lastSlash + 1);
        }

        // save image to history
        History history;
        m_currentImageName =
          history.packFileName("privateuploader",
                               upload[QStringLiteral("id")].toString(),
                               m_currentImageName);
        history.save(pixmap(), m_currentImageName);

        emit uploadOk(imageURL());
    } else {
        emit uploadError(reply);
    }
    new QShortcut(Qt::Key_Escape, this, SLOT(close()));
}

void PrivateUploader::upload()
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    pixmap().save(&buffer, "PNG");

    PrivateUploaderUpload* uploader = new PrivateUploaderUpload(this);
    connect(uploader,
            &PrivateUploaderUpload::uploadOk,
            [this, uploader, &buffer](QNetworkReply* reply) {
                handleReply(reply);
                uploader->deleteLater();
            });
    connect(uploader,
            &PrivateUploaderUpload::uploadError,
            this,
            &PrivateUploader::handleReply);
    const QString& fileName =
      FileNameHandler().parsedPattern().toLower().endsWith(".png")
        ? FileNameHandler().parsedPattern()
        : FileNameHandler().parsedPattern() + ".png";
    uploader->uploadBytes(byteArray, fileName, "image/png");
    buffer.close();
    byteArray.clear();

    connect(uploader,
            &PrivateUploaderUpload::uploadProgress,
            this,
            &PrivateUploader::updateProgress);
}

void PrivateUploader::deleteImage(const QString& fileName,
                                  const QString& deleteToken)
{
    Q_UNUSED(fileName)
    Q_UNUSED(deleteToken)
}

/*
void PrivateUploader::deleteImage(const QString& fileName, const QString&
deleteToken)
{
    Q_UNUSED(fileName)

    QUrl url(QString("%1/api/v3/gallery/%2").arg(ConfigHandler().serverTPU(),
deleteToken)); std::cout << url.toString().toStdString() << std::endl;

    QNetworkRequest request(url);
    request.setRawHeader("Authorization",
QString("%1").arg(ConfigHandler().uploadTokenTPU()).toUtf8());

    QNetworkReply *reply = m_NetworkAM->deleteResource(request);

    // Use QEventLoop to wait for the reply to finish
    QEventLoop loop;

    // Connect the finished signal to a lambda function
    connect(reply, &QNetworkReply::finished, [&]() {
        // Ensure reply is not nullptr before accessing
        if (reply && reply->error() == QNetworkReply::NoError) {
            qDebug() << "Image deleted successfully";
        } else {
            // Check if reply is nullptr to avoid dereferencing a null pointer
            qDebug() << "Error deleting image:" << (reply ? reply->errorString()
: "Reply is nullptr");
        }

        // Delete the reply and exit the event loop
        reply->deleteLater();
        loop.quit();
    });

    // Start the event loop
    loop.exec();

    // Emit the signal after the event loop has finished
    emit deleteOk();
}*/