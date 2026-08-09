/***************************************************************************
*                                                                         *
*   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "appicon/AppIcons.h"

#include <QGuiApplication>
#include <QImage>
#include <QScreen>

namespace {

QImage scaleImage(const QImage &source, int pixelSide, Qt::TransformationMode mode)
{
    if (mode != Qt::SmoothTransformation)
        return source.scaled(pixelSide, pixelSide, Qt::KeepAspectRatio, mode);

    QImage img = source.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    const QSize target = img.size().scaled(pixelSide, pixelSide, Qt::KeepAspectRatio);
    if (!target.isValid() || target.isEmpty())
        return img;

    while (img.width() >= target.width() * 2 && img.height() >= target.height() * 2)
        img = img.scaled(img.width() / 2, img.height() / 2,
                         Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    if (img.size() == target)
        return img;
    return img.scaled(target, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

} // namespace

qreal AppIcons::deviceRatio()
{
    qreal dpr = 1.0;
    const auto screens = QGuiApplication::screens();
    for (const QScreen *screen : screens) {
        if (screen)
            dpr = qMax(dpr, screen->devicePixelRatio());
    }
    return dpr;
}

QPixmap AppIcons::scale(const QPixmap &source, int logicalSide, Qt::TransformationMode mode)
{
    if (source.isNull() || logicalSide <= 0)
        return source;

    const qreal dpr = deviceRatio();
    const int pixelSide = qMax(1, qRound(logicalSide * dpr));

    if (source.width() == pixelSide && source.height() == pixelSide
            && qFuzzyCompare(source.devicePixelRatio(), dpr))
        return source;

    QPixmap result = QPixmap::fromImage(scaleImage(source.toImage(), pixelSide, mode));
    result.setDevicePixelRatio(dpr);
    return result;
}
