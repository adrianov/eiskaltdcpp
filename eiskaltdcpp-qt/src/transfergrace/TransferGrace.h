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

#pragma once

#include <QHash>
#include <QObject>
#include <QVariantMap>

/**
 * Timed hold before dropping idle Transfers rows.
 * Uploads: short Connected / complete grace. Downloads: Open-file minute after Finished.
 */
class TransferGrace : public QObject
{
    Q_OBJECT

public:
    explicit TransferGrace(QObject *parent = nullptr);

    void armUpload(const QVariantMap &params, int delayMs);
    void cancelUpload(const QString &cid, const QString &hub);
    void armDownload(const QString &target, int delayMs);
    void cancelDownload(const QString &target);

Q_SIGNALS:
    void uploadDue(QVariantMap params);
    void downloadDue(QString target);

private:
    static QString uploadKey(const QString &cid, const QString &hub);

    QHash<QString, int> uploadGen;
    QHash<QString, int> downloadGen;
};
