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

#include "transfersession/TransferSession.h"

#include "TransferDisplay.h"
#include "TransferViewModel.h"
#include "transfersession/TransferSessionRate.h"

#include "dcpp/stdinc.h"
#include "dcpp/TimerManager.h"

using namespace dcpp;

TransferSession::TransferSession(TransferViewItem *scope) :
    scope_(scope)
{
}

TransferViewItem *TransferSession::scopeOf(TransferViewItem *item, TransferViewItem *root)
{
    if (!item || !root)
        return nullptr;
    if (!item->cid.isEmpty() && item->parent() && item->parent() != root)
        return item->parent();
    return item;
}

qlonglong TransferSession::uploadMoved() const
{
    if (!scope_ || scope_->download)
        return 0;

    qlonglong total = scope_->fpos;
    if (scope_->cid.isEmpty()) {
        for (const auto &child : scope_->childItems) {
            if (!child->finished && !child->fail)
                total += child->segBytes;
        }
    } else if (!scope_->finished && !scope_->fail) {
        total += scope_->segBytes;
    }
    return total;
}

qlonglong TransferSession::downloadTotal() const
{
    if (!scope_ || !scope_->download)
        return 0;

    // File group: queue committed + in-flight per peer.
    if (scope_->cid.isEmpty()) {
        qlonglong total = scope_->fpos;
        for (const auto &child : scope_->childItems) {
            if (!child->fail)
                total += child->segBytes;
        }
        return TransferDisplay::highWaterBytes(scope_->dpos, total);
    }
    // Peer: finished segments + current in-flight.
    return scope_->fail ? scope_->fpos : (scope_->fpos + scope_->segBytes);
}

qlonglong TransferSession::movedBytes() const
{
    if (!scope_)
        return 0;
    if (scope_->download) {
        const qlonglong total = downloadTotal();
        // Peer: speedBase is left-at-join (size), not a subtracted baseline.
        if (!scope_->cid.isEmpty())
            return total > 0 ? total : 0;
        const qlonglong moved = total - scope_->speedBase;
        return moved > 0 ? moved : 0;
    }
    const qlonglong moved = uploadMoved();
    return moved > 0 ? moved : 0;
}

qlonglong TransferSession::progressBytes() const
{
    if (!scope_)
        return 0;
    if (scope_->download)
        return downloadTotal();
    return scope_->speedBase + uploadMoved();
}

void TransferSession::begin(qlonglong baseline)
{
    if (!scope_ || scope_->speedStart > 0)
        return;
    scope_->speedStart = GET_TICK();
    scope_->speedBase = baseline > 0 ? baseline : 0;
}

void TransferSession::writeUi(QVariantMap &params, qlonglong fileSize) const
{
    if (!scope_)
        return;

    // Download peer: speedBase = left-at-join; rate baseline is 0.
    const bool downloadPeer = scope_->download && !scope_->cid.isEmpty();
    qlonglong baseline = scope_->speedBase;
    if (downloadPeer) {
        baseline = 0;
        if (scope_->speedBase > 0)
            fileSize = scope_->speedBase;
    }
    if (fileSize <= 0)
        fileSize = scope_->data(COLUMN_TRANSFER_SIZE).toLongLong();

    const TransferSessionRate::Result rate = TransferSessionRate::compute({
            movedBytes(),
            baseline,
            fileSize,
            scope_->speedStart,
            GET_TICK()
    });

    double speed = TransferDisplay::roundSpeed(rate.bytesPerSec);
    // Warm-up / clock edge: keep last rate so Speed does not flicker to 0 B/s.
    if (speed <= 0.0 && rate.progress > 0 && scope_->speedStart > 0 && !scope_->finished) {
        const double prev = scope_->data(COLUMN_TRANSFER_SPEED).toDouble();
        if (prev > 0.0)
            speed = prev;
    }
    params["SPEED"] = speed;
    scope_->smoothTleft = TransferDisplay::smoothTimeLeft(scope_->smoothTleft, rate.etaSec);
    params["TLEFT"] = static_cast<qlonglong>(scope_->smoothTleft);
    params["DPOS"] = static_cast<qlonglong>(rate.progress);
    params["PERC"] = fileSize > 0
            ? qBound(0.0, rate.progress * 100.0 / fileSize, 100.0) : 0.0;
}

bool TransferSession::uploadDone() const
{
    if (!scope_ || scope_->download)
        return false;

    const qint64 size = scope_->data(COLUMN_TRANSFER_SIZE).toLongLong();
    if (size <= 0 || scope_->fpos < size)
        return false;

    if (scope_->cid.isEmpty()) {
        if (scope_->childCount() < 1)
            return false;
        for (const auto &child : scope_->childItems) {
            if (!child->finished && !child->fail)
                return false;
        }
        return true;
    }

    return scope_->finished || scope_->fail;
}
