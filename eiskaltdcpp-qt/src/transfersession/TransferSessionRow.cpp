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

#include "transfersession/TransferSessionRow.h"

#include "transferdisplay/TransferDisplay.h"
#include "TransferViewMetrics.h"
#include "TransferViewModel.h"
#include "transfersession/TransferSession.h"

#include "dcpp/stdinc.h"
#include "dcpp/TimerManager.h"

using namespace dcpp;

namespace {

qlonglong lng(const QVariant &v) { return v.toLongLong(); }
bool flag(const QVariant &v) { return v.toBool(); }
double dbl(const QVariant &v) { return v.toDouble(); }

} // namespace

void TransferSessionRow::commitSegment(TransferViewItem *item)
{
    if (!item || item->segBytes <= 0)
        return;
    item->fpos += item->segBytes;
    item->segBytes = 0;
    item->dpos = item->fpos;
}

void TransferSessionRow::trackPeer(TransferViewItem *item, const QVariantMap &params)
{
    if (!item || !flag(params.value("DOWN")))
        return;

    qlonglong pos = -1;
    if (params.contains("SEGP"))
        pos = lng(params.value("SEGP"));
    else if (params.contains("DPOS"))
        pos = lng(params.value("DPOS"));

    if (flag(params.value("SEGMENT_DONE")) || flag(params.value("FAIL"))) {
        if (item->segBytes > 0) {
            if (pos >= 0)
                item->segBytes = pos;
            commitSegment(item);
        }
        item->dpos = item->fpos;
        return;
    }

    if (pos >= 0) {
        // Segment rewind without Starting — keep the peer total continuous.
        if (pos < item->segBytes)
            commitSegment(item);
        item->segBytes = pos;
    }

    item->dpos = item->fpos + item->segBytes;
}

void TransferSessionRow::setQueuePos(TransferViewItem *item, TransferViewItem *root,
                                     const QVariantMap &params)
{
    if (!item || !root || !item->download || !params.contains("FPOS"))
        return;
    TransferViewItem *group = item->parent();
    if (!group || group == root || !root->childItems.contains(group))
        return;
    group->fpos = lng(params.value("FPOS"));
}

qlonglong TransferSessionRow::fileSizeOf(const QVariantMap &params, TransferViewItem *fallback)
{
    if (params.contains("ESIZE")) {
        const qlonglong n = lng(params.value("ESIZE"));
        if (n > 0)
            return n;
    }
    return fallback ? fallback->data(COLUMN_TRANSFER_SIZE).toLongLong() : 0;
}

qlonglong TransferSessionRow::leftAtJoin(qlonglong fileSize, qlonglong queueDone)
{
    if (fileSize <= 0)
        return 0;
    if (queueDone <= 0)
        return fileSize;
    if (fileSize > queueDone)
        return fileSize - queueDone;
    return 0;
}

void TransferSessionRow::beginFile(TransferViewItem *item, TransferViewItem *scope,
                                   const QVariantMap &params)
{
    if (!scope || scope->speedStart > 0)
        return;
    const qlonglong base = item->download
            ? (params.contains("FPOS") ? lng(params.value("FPOS")) : scope->fpos)
            : (params.contains("BASE") ? lng(params.value("BASE")) : 0);
    TransferSession(scope).begin(base);
}

void TransferSessionRow::publishPeer(TransferViewItem *item, TransferViewItem *scope,
                                     QVariantMap &params, const QString &downloadedPrefix)
{
    TransferSession peer(item);
    const qlonglong fileSize = fileSizeOf(params, scope);
    if (item->speedStart == 0) {
        const qlonglong queueDone = params.contains("FPOS")
                ? lng(params.value("FPOS"))
                : (scope ? scope->fpos : 0);
        // speedBase = file left at join (size for % / ETA), not a byte baseline.
        item->speedStart = GET_TICK();
        item->speedBase = leftAtJoin(fileSize, queueDone);
        item->smoothTleft = -1;
    }

    const qlonglong left = item->speedBase > 0 ? item->speedBase : fileSize;
    peer.writeUi(params, left);
    item->updateColumn(COLUMN_TRANSFER_SPEED, params.value("SPEED"));
    item->updateColumn(COLUMN_TRANSFER_TLEFT, params.value("TLEFT"));
    item->dpos = lng(params.value("DPOS"));
    item->percent = qBound(0.0, dbl(params.value("PERC")), 100.0);

    const QString stat = item->data(COLUMN_TRANSFER_STATS).toString();
    if (item->dpos > 0 || stat.startsWith(downloadedPrefix)) {
        item->updateColumn(COLUMN_TRANSFER_STATS,
                TransferViewMetrics::downloadProgressStat(item->dpos, left));
    }
}

void TransferSessionRow::publishUpload(TransferViewItem *item, TransferViewItem *scope,
                                       QVariantMap &params, const QString &downloadedPrefix,
                                       const QString &uploadedPrefix)
{
    TransferSession(scope).writeUi(params, fileSizeOf(params, scope));
    item->updateColumn(COLUMN_TRANSFER_SPEED, params.value("SPEED"));
    item->updateColumn(COLUMN_TRANSFER_TLEFT, params.value("TLEFT"));
    if (item->download)
        return;

    item->dpos = lng(params.value("DPOS"));
    item->percent = qBound(0.0, dbl(params.value("PERC")), 100.0);
    if (item->data(COLUMN_TRANSFER_SIZE).toLongLong() <= 0 && params.contains("ESIZE"))
        item->updateColumn(COLUMN_TRANSFER_SIZE, params.value("ESIZE"));

    const QString stat = item->data(COLUMN_TRANSFER_STATS).toString();
    if (stat.isEmpty()
            || TransferDisplay::isProgressStat(stat, downloadedPrefix, uploadedPrefix)
            || flag(params.value("SOFT_STAT"))) {
        item->updateColumn(COLUMN_TRANSFER_STATS,
                TransferViewMetrics::uploadProgressStat(
                        item->dpos, item->data(COLUMN_TRANSFER_SIZE).toLongLong()));
    }
    if (scope != item) {
        scope->dpos = item->dpos;
        scope->percent = item->percent;
    }
}

void TransferSessionRow::publish(TransferViewItem *item, TransferViewItem *root,
                                 QVariantMap &params, const QString &downloadedPrefix,
                                 const QString &uploadedPrefix)
{
    if (!item || item->fail)
        return;

    TransferViewItem *scope = TransferSession::scopeOf(item, root);
    if (!scope)
        return;
    if (scope->speedStart == 0 && !params.contains("SEGP") && item->speedStart == 0)
        return;

    beginFile(item, scope, params);

    // Download peer: session vs bytes left when this peer first joined.
    if (item->download && scope != item) {
        publishPeer(item, scope, params, downloadedPrefix);
        return;
    }

    publishUpload(item, scope, params, downloadedPrefix, uploadedPrefix);
}
