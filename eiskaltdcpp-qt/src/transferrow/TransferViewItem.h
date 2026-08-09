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

#include <QList>
#include <QString>
#include <QVariant>

static const int COLUMN_TRANSFER_USERS       = 0;
static const int COLUMN_TRANSFER_SPEED       = 1;
static const int COLUMN_TRANSFER_STATS       = 2;
static const int COLUMN_TRANSFER_FLAGS       = 3;
static const int COLUMN_TRANSFER_SIZE        = 4;
static const int COLUMN_TRANSFER_TLEFT       = 5;
static const int COLUMN_TRANSFER_FNAME       = 6;
static const int COLUMN_TRANSFER_HOST        = 7;
static const int COLUMN_TRANSFER_TAG         = 8;
static const int COLUMN_TRANSFER_IP          = 9;
static const int COLUMN_TRANSFER_ENCRYPTION  = 10;

/** One Transfers-list row: peer leaf or file group parent. */
class TransferViewItem
{
public:
    TransferViewItem(const QList<QVariant> &data, TransferViewItem *parent = nullptr);
    virtual ~TransferViewItem();

    void appendChild(TransferViewItem *child);

    TransferViewItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    TransferViewItem *parent();
    void updateColumn(int, QVariant);

    /** Finished download group: keep for Open-file grace / Keep downloaded files. */
    bool holdFinished() const { return download && finished; }

    QList<TransferViewItem*> childItems;

    bool download;
    bool fail;
    bool finished;
    QString cid;
    QString tth;
    QString target;
    /** Shown bytes: download peer = lifetime total; download parent = file aggregate. */
    qlonglong dpos;
    /** Download parent: queue committed. Download peer / upload: finished segment bytes. */
    qlonglong fpos;
    /** Current segment in flight (Download/Upload::getPos). */
    qlonglong segBytes;
    /** Session begin tick (GET_TICK); see transfersession/. */
    quint64 speedStart;
    /** File/upload session: baseline at begin. Download peer: file left at join. */
    qlonglong speedBase;
    double percent;
    /** Shown Time left (seconds), smoothed — see TransferDisplay::smoothTimeLeft. */
    qlonglong smoothTleft;
    /** Non-zero when Download complete: newer ranks sort above older grace rows. */
    quint64 finishRank;
    QList<QVariant> itemData;

private:
    TransferViewItem *parentItem;
};
