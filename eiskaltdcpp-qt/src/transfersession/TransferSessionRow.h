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

#include <QString>
#include <QVariantMap>

class TransferViewItem;

/**
 * Binds TransferSession counters to a Transfer View row: peer segment totals,
 * queue position on the file group, and SPEED / % / ETA columns.
 */
class TransferSessionRow
{
public:
    /** Fold finished in-flight bytes into the peer total (`fpos`). */
    static void commitSegment(TransferViewItem *item);
    /** Track download peer in-flight bytes; commit on end / fail / rewind. */
    static void trackPeer(TransferViewItem *item, const QVariantMap &params);
    /** Queue-committed position on the download file-group parent. */
    static void setQueuePos(TransferViewItem *item, TransferViewItem *root,
                            const QVariantMap &params);
    /** Write SPEED / TLEFT / % / progress text for the row. */
    static void publish(TransferViewItem *item, TransferViewItem *root,
                        QVariantMap &params, const QString &downloadedPrefix,
                        const QString &uploadedPrefix);

private:
    static qlonglong fileSizeOf(const QVariantMap &params, TransferViewItem *fallback);
    /** Whole-file bytes still left when the peer's first segment starts. */
    static qlonglong leftAtJoin(qlonglong fileSize, qlonglong queueDone);
    static void beginFile(TransferViewItem *item, TransferViewItem *scope,
                          const QVariantMap &params);
    static void publishPeer(TransferViewItem *item, TransferViewItem *scope,
                            QVariantMap &params, const QString &downloadedPrefix);
    static void publishUpload(TransferViewItem *item, TransferViewItem *scope,
                              QVariantMap &params, const QString &downloadedPrefix,
                              const QString &uploadedPrefix);
};
