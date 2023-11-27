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
#include <QApplication>
#include <QStyle>
// FIXME #include <QBuffer>
#include <QClipboard>
#include <QCursor>
#include <QDesktopServices>
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

#define WINDOW_WIDTH 440
#define WINDOW_HEIGHT 120

ImgUploaderBase::ImgUploaderBase(const QPixmap& capture, QWidget* parent)
  : QWidget(parent)
  , m_pixmap(capture)
{
    FlameshotDaemon::copyToClipboard("");
    if (!ConfigHandler().uploadWindowEnabled()) {
        return;
    }
    setWindowTitle(tr("Upload image"));
    setWindowIcon(QIcon(GlobalValues::iconPath()));

    setAttribute(Qt::WA_ShowWithoutActivating);
    setWindowFlags(Qt::BypassWindowManagerHint | Qt::WindowStaysOnTopHint |
                   Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus |
                   Qt::Tool);

    m_infoLabel = new QLabel(tr("Uploading image..."));
    m_infoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_infoLabel->setCursor(QCursor(Qt::IBeamCursor));

    // get screens
    QList<QScreen*> screens = QGuiApplication::screens();
    QRect totalResolution;

    for (QScreen* screen : screens) {
        totalResolution = totalResolution.united(screen->geometry());
    }

    setFixedSize(WINDOW_WIDTH * 1.5, WINDOW_HEIGHT);
    move(totalResolution.bottomRight().x() - (WINDOW_WIDTH * 1.5) -
           ConfigHandler().uploadWindowOffsetX(),
         totalResolution.bottomRight().y() - WINDOW_HEIGHT -
           ConfigHandler().uploadWindowOffsetY());
    show();

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

    m_closeTimer = new QTimer(this);
    m_closeTimer->setSingleShot(true);
    m_closeTimer->setInterval(ConfigHandler().uploadWindowTimeout());
    connect(m_closeTimer, &QTimer::timeout, this, &QWidget::close);
}

// Handle pause/start of timer when hovering over the Qt widget
void ImgUploaderBase::enterEvent(QEvent *event) {
    if ((m_closeTimer != nullptr) && m_closeTimer->isActive() && m_hasUploaded) {
        m_remainingTimeOnPause = m_closeTimer->remainingTime();
        m_closeTimer->stop();
    }
    QWidget::enterEvent(event);
}

void ImgUploaderBase::leaveEvent(QEvent *event) {
    if ((m_closeTimer != nullptr) && !m_closeTimer->isActive() && m_hasUploaded) {
        m_closeTimer->start(m_remainingTimeOnPause);
        m_remainingTimeOnPause = -1;
    }
    QWidget::leaveEvent(event);
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

void ImgUploaderBase::setInfoLabelText(const QString& text)
{
    m_infoLabel->setText(text);
}

void ImgUploaderBase::startDrag()
{
    //
}

void ImgUploaderBase::showPreUploadDialog(int open)
{
    if (!ConfigHandler().uploadWindowEnabled()) {
        return;
    }
    int padding =
      open - 1 == 0 ? 0 : ConfigHandler().uploadWindowStackPadding();
    int offset = (open - 1) * (WINDOW_HEIGHT + padding);
    move(QPoint(x(), y() - offset));
}

void ImgUploaderBase::updateProgress(int percentage)
{
    if (!m_infoLabel || !ConfigHandler().uploadWindowEnabled()) {
        return;
    }

    m_infoLabel->setText(tr("Uploading image... %1%").arg(percentage));
}

void ImgUploaderBase::showPostUploadDialog(int open) {
    copyURL();
    m_hasUploaded = true;
    if (!ConfigHandler().uploadWindowEnabled()) {
        destroyed(this);
        return;
    }

    m_closeTimer->start();

    m_infoLabel->deleteLater();

    QHBoxLayout* imageAndUrlLayout = new QHBoxLayout;

    auto* imageLabel = new ImageLabel();
    imageLabel->setScreenshot(m_pixmap);
    imageLabel->setFixedSize(WINDOW_WIDTH / 2.5, WINDOW_HEIGHT - 20);
    imageLabel->setCursor(QCursor(Qt::PointingHandCursor));

    imageAndUrlLayout->addWidget(imageLabel, 0, Qt::AlignCenter);

    QVBoxLayout* urlAndButtonsLayout = new QVBoxLayout;


    QLabel* label = new QLabel(m_imageURL.toString());
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QHBoxLayout* closeButtonAndLabelLayout = new QHBoxLayout;

    closeButtonAndLabelLayout->addWidget(label);
    closeButtonAndLabelLayout->addWidget(m_closeButton);
    closeButtonAndLabelLayout->setAlignment(m_closeButton, Qt::AlignRight);
    closeButtonAndLabelLayout->setAlignment(label, Qt::AlignLeft);


    urlAndButtonsLayout->addLayout(closeButtonAndLabelLayout);

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    m_copyUrlButton = new QPushButton(tr("Copy URL"));
    m_openUrlButton = new QPushButton(tr("Open URL"));
    m_toClipboardButton = new QPushButton(tr("Copy Image"));
    m_saveToFilesystemButton = new QPushButton(tr("Save Image"));

    buttonsLayout->addWidget(m_copyUrlButton);
    buttonsLayout->addWidget(m_openUrlButton);
    buttonsLayout->addWidget(m_toClipboardButton);
    buttonsLayout->addWidget(m_saveToFilesystemButton);

    connect(m_copyUrlButton, &QPushButton::clicked, this, &ImgUploaderBase::copyURL);
    connect(m_openUrlButton, &QPushButton::clicked, this, &ImgUploaderBase::openURL);
    connect(m_toClipboardButton, &QPushButton::clicked, this, &ImgUploaderBase::copyImage);
    connect(m_saveToFilesystemButton, &QPushButton::clicked, this, &ImgUploaderBase::saveScreenshotToFilesystem);

    urlAndButtonsLayout->addLayout(buttonsLayout);
    imageAndUrlLayout->addLayout(urlAndButtonsLayout);

    m_vLayout->addLayout(imageAndUrlLayout);
}

void ImgUploaderBase::openURL()
{
    bool successful =   QDesktopServices::openUrl(m_imageURL);
}

void ImgUploaderBase::copyURL()
{
    FlameshotDaemon::copyToClipboard(m_imageURL.toString());
}

void ImgUploaderBase::copyImage()
{
    FlameshotDaemon::copyToClipboard(m_pixmap);
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
        return;
    }
}