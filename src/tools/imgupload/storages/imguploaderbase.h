// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#pragma once

#include <QUrl>
#include <QWidget>

class QNetworkReply;
class QNetworkAccessManager;
class QHBoxLayout;
class QVBoxLayout;
class QLabel;
class LoadSpinner;
class QPushButton;
class QUrl;
class NotificationWidget;

class ImgUploaderBase : public QWidget
{
    Q_OBJECT

public:
    explicit ImgUploaderBase(const QPixmap& capture, QWidget* parent = nullptr);

    LoadSpinner* spinner();

    const QUrl& imageURL();
    void setImageURL(const QUrl&);
    const QPixmap& pixmap();
    void setPixmap(const QPixmap&);
    void setInfoLabelText(const QString&);

    virtual void deleteImage(const QString& fileName,
                             const QString& deleteToken) = 0;
    virtual void upload() = 0;

signals:
    void uploadOk(const QUrl& url);
    void deleteOk();
    void uploadProgress(int progress);
    void uploadError(QNetworkReply* error);

public slots:
    void showPostUploadDialog(int open);
    void showPreUploadDialog(int open);
    void updateProgress(int percentage);

private slots:
    void startDrag();
    void copyURL();
    void openURL();
    void copyImage();
    void deleteCurrentImage();
    void saveScreenshotToFilesystem();

private:
    QPixmap m_pixmap;

    QVBoxLayout* m_vLayout;
    QHBoxLayout* m_hLayout;
    // loading
    QLabel* m_infoLabel;
    LoadSpinner* m_spinner;
    // uploaded
    QPushButton* m_openUrlButton;
    QPushButton* m_openDeleteUrlButton;
    QPushButton* m_copyUrlButton;
    QPushButton* m_toClipboardButton;
    QPushButton* m_saveToFilesystemButton;
    QPushButton* m_closeButton;
    QUrl m_imageURL;
    NotificationWidget* m_notification;
    QTimer* m_closeTimer;
    QPushButton* m_retryButton = nullptr;
    bool m_hasUploaded;
    int m_remainingTimeOnPause = -1;
    QLabel* m_label;
    void usePrimaryScreen();

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

public:
    QString m_currentImageName;
    void showErrorUploadDialog(QNetworkReply* error);
};
