/***************************************************************************
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
    if (!item->download || !group->download || !group->cid.isEmpty())
        return false;
    if (group->childCount() != 1 || group->childItems.first() != item)
        return false;

    group->target = newTarget;
    group->finished = false;
    group->fail = false;
    group->percent = 0.0;
    group->smoothTleft = -1;
    group->fpos = p.contains("FPOS") ? p.value("FPOS").toLongLong() : 0;
    group->dpos = group->fpos;
    if (p.contains("ESIZE"))
        group->updateColumn(COLUMN_TRANSFER_SIZE, p.value("ESIZE"));
    if (p.contains("FNAME"))
        group->updateColumn(COLUMN_TRANSFER_FNAME, p.value("FNAME"));
    if (p.contains("USER"))
        group->updateColumn(COLUMN_TRANSFER_USERS, p.value("USER"));
    return true;
}

} // namespace TransferViewTree
