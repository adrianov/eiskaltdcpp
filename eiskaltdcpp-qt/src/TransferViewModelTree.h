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

#include "TransferViewModel.h"

namespace TransferViewTree {

inline bool isHiddenName(const QString &fname)
{
    return fname.isEmpty() || fname == TransferViewModel::tr("File list");
}

inline bool isAttached(TransferViewItem *item)
{
    return item && item->parent() && item->parent()->childItems.contains(item);
}

inline bool targetChanged(const TransferViewItem *item, const QString &newTarget)
{
    return item && !item->target.isEmpty() && !newTarget.isEmpty() && newTarget != item->target;
}

inline void clearByteProgress(TransferViewItem *item)
{
    if (!item)
        return;
    item->fpos = 0;
    item->dpos = 0;
    item->segBytes = 0;
    item->speedStart = 0;
    item->speedBase = 0;
    item->smoothTleft = -1;
}

inline bool scopeFullyDone(const TransferViewItem *scope)
{
    if (!scope || !scope->finished)
        return false;
    if (scope->percent >= 100.0)
        return true;
    const qlonglong size = scope->data(COLUMN_TRANSFER_SIZE).toLongLong();
    return size > 0 && scope->fpos >= size;
}

inline void attach(TransferViewItem *item, TransferViewItem *parent)
{
    if (!item || !parent || isAttached(item))
        return;
    parent->appendChild(item);
}

inline bool wantsParent(const QVariantMap &params, const QString &fname)
{
    if (fname == TransferViewModel::tr("File list") || params.value("TARGET").toString().isEmpty())
        return false;
    if (params.value("DOWN").toBool())
        return true;
    return !params.value("IP").toString().isEmpty();
}

/** Next file on the same sole connection: keep the group row identity. */
inline bool retargetGroup(TransferViewItem *item, TransferViewItem *group,
                          const QString &newTarget, const QVariantMap &p)
{
    if (!item || !group || newTarget.isEmpty() || group->target == newTarget)
        return false;
    if (item->download != group->download || !group->cid.isEmpty())
        return false;
    if (group->childCount() != 1 || group->childItems.first() != item)
        return false;

    group->target = newTarget;
    group->finished = false;
    group->finishRank = 0;
    group->fail = false;
    clearByteProgress(group);
    group->percent = 0.0;
    // Downloads: queue FPOS. Uploads: reset finished-segment counter for the new file.
    group->fpos = item->download && p.contains("FPOS") ? p.value("FPOS").toLongLong() : 0;
    group->dpos = group->fpos;
    clearByteProgress(item);
    if (p.contains("ESIZE"))
        group->updateColumn(COLUMN_TRANSFER_SIZE, p.value("ESIZE"));
    if (p.contains("FNAME"))
        group->updateColumn(COLUMN_TRANSFER_FNAME, p.value("FNAME"));
    if (p.contains("USER"))
        group->updateColumn(COLUMN_TRANSFER_USERS, p.value("USER"));
    if (p.contains("TAG"))
        group->updateColumn(COLUMN_TRANSFER_TAG, p.value("TAG"));
    return true;
}

} // namespace TransferViewTree
