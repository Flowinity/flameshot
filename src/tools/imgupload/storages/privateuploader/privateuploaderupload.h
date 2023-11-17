// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#pragma once

#include "src/tools/imgupload/storages/imguploaderbase.h"
#include <QUrl>
#include <QWidget>

class QNetworkReply;
class QNetworkAccessManager;
class QUrl;

class PrivateUploaderUpload : public QObject
{
    Q_OBJECT

public:
    PrivateUploaderUpload(QObject* parent = nullptr);

public slots:
    void uploadBytes(const QByteArray& byteArray,
                     const QString& fileName,
                     const QString& fileType);

signals:
    void uploadOk(QNetworkReply* reply);
    void uploadError(const QString& error);
    void uploadProgress(int progress);

private:
    QNetworkAccessManager* m_NetworkAM;
};