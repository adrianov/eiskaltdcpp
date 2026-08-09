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

#include "transfersession/TransferGroup.h"

#include "transferdisplay/TransferDisplay.h"
#include "TransferViewModel.h"
#include "WulforUtil.h"
#include "transfersession/TransferSession.h"

#include <QVariantMap>

TransferGroup::TransferGroup(TransferViewItem *parent) :
    parent_(parent)
{
}

bool TransferGroup::valid() const
{
    return parent_ && parent_->parent() && parent_->childCount() > 0;
}

void TransferGroup::refresh()
{
    if (!valid())
        return;

    Scan s;
    s.totalSize = parent_->data(COLUMN_TRANSFER_SIZE).toLongLong();
    for (const auto &child : parent_->childItems)
        scanChild(child, s);
    finishProgress(s);
    writeSpeed(s);
    writeIdentity(s);
}

void TransferGroup::scanChild(TransferViewItem *child, Scan &s) const
{
    if (!child->fail)
        noteActive(child, s);

    const QString hub = child->data(COLUMN_TRANSFER_HOST).toString();
    if (!hub.isEmpty() && !s.hubs.contains(hub))
        s.hubs.append(hub);

    const QString tag = child->data(COLUMN_TRANSFER_TAG).toString();
    if (!tag.isEmpty() && !s.tags.contains(tag))
        s.tags.append(tag);

    // Prefer a peer that is still trying; a parked one only until a better arrives.
    const QString stat = child->data(COLUMN_TRANSFER_STATS).toString();
    if (!stat.isEmpty() && (s.childStat.isEmpty() || (s.statFailed && !child->fail))) {
        s.childStat = stat;
        s.statFailed = child->fail;
    }

    const qint64 sz = child->data(COLUMN_TRANSFER_SIZE).toLongLong();
    if (sz > s.totalSize)
        s.totalSize = sz;
    // In-flight only — peer lifetime totals stay on the child row (fpos + segBytes).
    if (parent_->download)
        s.progressPos += child->segBytes;
    if (s.nick.isEmpty())
        s.nick = child->data(COLUMN_TRANSFER_USERS).toString();
}

void TransferGroup::noteActive(TransferViewItem *child, Scan &s) const
{
    if (parent_->download) {
        if (child->segBytes > 0)
            s.active++;
        return;
    }
    // In-flight upload segment (finished means idle between segments).
    if (!child->finished
            && !child->data(COLUMN_TRANSFER_FNAME).toString().isEmpty())
        s.active++;
}

void TransferGroup::finishProgress(Scan &s) const
{
    if (parent_->download)
        s.progressPos = downloadProgress(s);
    else {
        s.progressPos = TransferSession(parent_).progressBytes();
        if (s.totalSize > 0 && s.progressPos > s.totalSize)
            s.progressPos = s.totalSize;
    }
    parent_->dpos = s.progressPos;
    if (s.totalSize > 0)
        parent_->updateColumn(COLUMN_TRANSFER_SIZE, s.totalSize);
    // Finished downloads stay at 100% even if peer counters lag the final size.
    if (parent_->finished) {
        if (s.totalSize > 0)
            parent_->dpos = s.totalSize;
        parent_->percent = 100.0;
        return;
    }
    parent_->percent = s.totalSize > 0
            ? qBound(0.0, parent_->dpos * 100.0 / s.totalSize, 100.0) : 0.0;
}

qlonglong TransferGroup::downloadProgress(const Scan &s) const
{
    qlonglong pos = s.progressPos + parent_->fpos;
    if (s.totalSize > 0 && pos > s.totalSize)
        pos = s.totalSize;
    if (!parent_->finished)
        pos = TransferDisplay::highWaterBytes(parent_->dpos, pos);
    return pos;
}

void TransferGroup::writeSpeed(const Scan &s) const
{
    QVariantMap params;
    TransferSession(parent_).writeUi(params, s.totalSize);
    double speed = params.value("SPEED").toDouble();
    qint64 timeLeft = params.value("TLEFT").toLongLong();
    if (timeLeft < 0)
        timeLeft = 0; // display 00:00:00 until the first estimate
    parent_->updateColumn(COLUMN_TRANSFER_SPEED, speed);
    parent_->updateColumn(COLUMN_TRANSFER_TLEFT, timeLeft);
    if (!parent_->finished)
        parent_->updateColumn(COLUMN_TRANSFER_STATS, progressStat(s));
}

QString TransferGroup::progressStat(const Scan &s) const
{
    // Byte counters would read "0 B (0.0%)" before the first byte — echo the peers instead.
    if (parent_->download && !s.active && parent_->dpos <= 0 && !s.childStat.isEmpty())
        return s.childStat;
    return (parent_->download ? TransferViewModel::tr("Downloaded ")
                              : TransferViewModel::tr("Uploaded "))
            + WulforUtil::formatBytes(parent_->dpos)
            + QString(" (%1%)").arg(parent_->percent, 0, 'f', 1);
}

void TransferGroup::writeIdentity(const Scan &s) const
{
    QString name = parent_->data(COLUMN_TRANSFER_FNAME).toString();
    if (name.startsWith(QLatin1String("TTH: "))) {
        name.remove(0, 5);
        parent_->updateColumn(COLUMN_TRANSFER_FNAME, name);
    }
    parent_->updateColumn(COLUMN_TRANSFER_USERS,
            parent_->childCount() > 1 && !s.nick.isEmpty()
                ? s.nick + QString(", ... (%1/%2)").arg(s.active).arg(parent_->childCount())
                : s.nick);
    parent_->updateColumn(COLUMN_TRANSFER_FLAGS, QString());
    parent_->updateColumn(COLUMN_TRANSFER_HOST, joinList(s.hubs));
    parent_->updateColumn(COLUMN_TRANSFER_TAG, joinList(s.tags));
}

QString TransferGroup::joinList(const QList<QString> &parts)
{
    QString out;
    for (const QString &part : parts)
        out += part + QLatin1Char(' ');
    return out;
}
