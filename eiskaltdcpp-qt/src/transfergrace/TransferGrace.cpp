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

#include "transfergrace/TransferGrace.h"

#include <QTimer>

TransferGrace::TransferGrace(QObject *parent)
    : QObject(parent)
{
}

QString TransferGrace::uploadKey(const QString &cid, const QString &hub) {
    return cid + QLatin1Char('|') + hub;
}

void TransferGrace::cancelUpload(const QString &cid, const QString &hub) {
    if (!cid.isEmpty())
        ++uploadGen[uploadKey(cid, hub)];
}

void TransferGrace::armUpload(const QVariantMap &params, int delayMs) {
    if (params.value(QStringLiteral("DOWN")).toBool()
            || params.value(QStringLiteral("CID")).toString().isEmpty()
            || delayMs < 0)
        return;

    const QString key = uploadKey(params.value(QStringLiteral("CID")).toString(),
                                  params.value(QStringLiteral("HOST")).toString());
    const int gen = ++uploadGen[key];
    QTimer::singleShot(delayMs, this, [this, key, gen, params]() {
        if (uploadGen.value(key) == gen)
            emit uploadDue(params);
    });
}

void TransferGrace::cancelDownload(const QString &target) {
    if (!target.isEmpty())
        ++downloadGen[target];
}

void TransferGrace::armDownload(const QString &target, int delayMs) {
    if (target.isEmpty() || delayMs < 0)
        return;
    const int gen = ++downloadGen[target];
    QTimer::singleShot(delayMs, this, [this, target, gen]() {
        if (downloadGen.value(target) == gen)
            emit downloadDue(target);
    });
}
