// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "imguploaderbase.h"
#include "src/core/flameshotdaemon.h"
#include "src/utils/confighandler.h"
#include "src/utils/globalvalues.h"
#include "src/utils/history.h"
#include "src/utils/screenshotsaver.h"
#include "src/widgets/imagelabel.h"
#include "src/widgets/loadspinner.h"
#include "src/widgets/notificationwidget.h"
#include <QApplication>
// FIXME #include <QBuffer>
#include <QClipboard>
#include <QCursor>
#include <QDesktopServices>
#include <QDrag>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMimeData>
#include <QNetworkAccessManager>
#include <QPushButton>
#include <QRect>
#include <QScreen>
#include <QShortcut>
#include <QTimer>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <iostream>

#define CLOSE_TIMEOUT 25000

ImgUploaderBase::ImgUploaderBase(const QPixmap& capture, QWidget* parent)
  : QWidget(parent)
  , m_pixmap(capture)
{
    setWindowTitle(tr("Upload image"));
    setWindowIcon(QIcon(GlobalValues::iconPath()));

    QRect position = frameGeometry();
    QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
    position.moveBottomRight(screen->availableGeometry().bottomRight());
    move(position.topLeft());
    setAttribute(Qt::WA_ShowWithoutActivating);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowSystemMenuHint);

    m_infoLabel = new QLabel(tr("Uploading Image"));
    m_infoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_infoLabel->setCursor(QCursor(Qt::IBeamCursor));

    m_closeButton = new QPushButton(tr("X"));
    m_closeButton->setFixedSize(20, 20);
    m_closeButton->setCursor(QCursor(Qt::PointingHandCursor));
    m_closeButton->setToolTip(tr("Close"));
    m_closeButton->setStyleSheet("QPushButton {"
                                 "border: none;"
                                 "background-color: transparent;"
                                 "color: white;"
                                 "font-size: 16px;"
                                 "float: right;"
                                 "}");

    connect(m_closeButton, &QPushButton::clicked, this, &QWidget::close);



    m_vLayout = new QVBoxLayout();
    setLayout(m_vLayout);
    m_vLayout->addWidget(m_infoLabel);

    setAttribute(Qt::WA_DeleteOnClose);
    QTimer::singleShot(CLOSE_TIMEOUT, this, &QWidget::close);
}

void ImgUploaderBase::contextMenuEvent(QContextMenuEvent* event)
{
    QWidget::close();
}

const QUrl& ImgUploaderBase::imageURL()
{
    return m_imageURL;
}

void ImgUploaderBase::setImageURL(const QUrl& imageURL)
{
    m_imageURL = imageURL;
}

const QPixmap& ImgUploaderBase::pixmap()
{
    return m_pixmap;
}

void ImgUploaderBase::setPixmap(const QPixmap& pixmap)
{
    m_pixmap = pixmap;
}

NotificationWidget* ImgUploaderBase::notification()
{
    return m_notification;
}

void ImgUploaderBase::setInfoLabelText(const QString& text)
{
    m_infoLabel->setText(text);
}

void ImgUploaderBase::startDrag()
{
    //
}

#define WINDOW_HEIGHT 140

void ImgUploaderBase::showPostUploadDialog(int open)
{
    int offset = (open - 1) * WINDOW_HEIGHT;
    move(QPoint(x(), y() - offset));

    m_infoLabel->deleteLater();

    m_notification = new NotificationWidget();
    m_vLayout->addWidget(m_notification);

    QHBoxLayout* hLayoutLabel = new QHBoxLayout;

    QLabel* label = new QLabel(m_imageURL.toString());
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);

    hLayoutLabel->addWidget(label);


    hLayoutLabel->addWidget(m_closeButton);

    // Add the horizontal layout to the main vertical layout
    m_vLayout->addLayout(hLayoutLabel);

    m_hLayout = new QHBoxLayout();
    m_vLayout->addLayout(m_hLayout);

    m_copyUrlButton = new QPushButton(tr("Copy URL"));
    m_openUrlButton = new QPushButton(tr("Open URL"));
    //m_openDeleteUrlButton = new QPushButton(tr("Delete image"));
    m_toClipboardButton = new QPushButton(tr("Copy Image"));
    m_saveToFilesystemButton = new QPushButton(tr("Save Image"));
    m_hLayout->addWidget(m_copyUrlButton);
    m_hLayout->addWidget(m_openUrlButton);
    //m_hLayout->addWidget(m_openDeleteUrlButton);
    m_hLayout->addWidget(m_toClipboardButton);
    m_hLayout->addWidget(m_saveToFilesystemButton);

    connect(
      m_copyUrlButton, &QPushButton::clicked, this, &ImgUploaderBase::copyURL);
    connect(
      m_openUrlButton, &QPushButton::clicked, this, &ImgUploaderBase::openURL);
    /*connect(m_openDeleteUrlButton,
            &QPushButton::clicked,
            this,
            &ImgUploaderBase::deleteCurrentImage);*/
    connect(m_toClipboardButton,
            &QPushButton::clicked,
            this,
            &ImgUploaderBase::copyImage);

    QObject::connect(m_saveToFilesystemButton,
                     &QPushButton::clicked,
                     this,
                     &ImgUploaderBase::saveScreenshotToFilesystem);

    copyURL();
}

void ImgUploaderBase::openURL()
{
    bool successful = QDesktopServices::openUrl(m_imageURL);
    if (!successful) {
        m_notification->showMessage(tr("Unable to open the URL."));
    }
}

void ImgUploaderBase::copyURL()
{
    FlameshotDaemon::copyToClipboard(m_imageURL.toString());
    //m_notification->showMessage(tr("URL copied to clipboard."));
}

void ImgUploaderBase::copyImage()
{
    FlameshotDaemon::copyToClipboard(m_pixmap);
    m_notification->showMessage(tr("Screenshot copied to clipboard."));
}

void ImgUploaderBase::deleteCurrentImage()
{
    History history;
    HistoryFileName unpackFileName = history.unpackFileName(m_currentImageName);
    deleteImage(unpackFileName.file, unpackFileName.token);
}

void ImgUploaderBase::saveScreenshotToFilesystem()
{
    if (!saveToFilesystemGUI(m_pixmap)) {
        m_notification->showMessage(
          tr("Unable to save the screenshot to disk."));
        return;
    }
    m_notification->showMessage(tr("Screenshot saved."));
}
