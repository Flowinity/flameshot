// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#pragma once

#include "src/tools/imgupload/storages/imguploaderbase.h"
#include <QUrl>
#include <QWidget>

class QNetworkReply;
class QNetworkAccessManager;
class QUrl;

class PrivateUploader : public ImgUploaderBase
{
    Q_OBJECT
public:
    explicit PrivateUploader(const QPixmap& capture, QWidget* parent = nullptr);
    void deleteImage(const QString& fileName, const QString& deleteToken);
    void uploadBytes(const QByteArray& bytes);

private slots:
    void handleReply(QNetworkReply* reply);

private:
    QNetworkAccessManager* m_NetworkAM;
    void upload();
};
