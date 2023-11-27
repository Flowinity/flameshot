// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "flameshotdbusadapter.h"
#include "src/core/flameshot.h"
#include "src/core/flameshotdaemon.h"
#include <QDateTime>

FlameshotDBusAdapter::FlameshotDBusAdapter(QObject* parent)
  : QDBusAbstractAdaptor(parent)
{}

FlameshotDBusAdapter::~FlameshotDBusAdapter() = default;

void FlameshotDBusAdapter::attachScreenshotToClipboard(const QByteArray& data)
{
    FlameshotDaemon::instance()->attachScreenshotToClipboard(data);
}

void FlameshotDBusAdapter::attachTextToClipboard(const QString& text,
                                                 const QString& notification)
{
    FlameshotDaemon::instance()->attachTextToClipboard(text, notification);
}

void FlameshotDBusAdapter::attachPin(const QByteArray& data)
{
    FlameshotDaemon::instance()->attachPin(data);
}

void FlameshotDBusAdapter::captureScreen(const QString& captureMode)
{
#ifdef MEASURE_INIT_TIME
    qputenv("FLAMESHOT_INIT_TIME", QByteArray::number(QDateTime::currentMSecsSinceEpoch()));
#endif
    int const captureModeInt = captureMode.toInt();
    if (captureModeInt < 0 || captureModeInt > 3) {
        return;
    }
    Flameshot::instance()->requestCapture(
      CaptureRequest::CaptureMode(captureModeInt));
}