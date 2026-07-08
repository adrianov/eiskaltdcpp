/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferView.h"
#include "TransferViewModel.h"
#include "WulforUtil.h"

#include "dcpp/CID.h"
#include "dcpp/ClientManager.h"
#include "dcpp/QueueManager.h"
#include "dcpp/ConnectionManager.h"

QString TransferView::resolveDownloadTarget(TransferViewItem *item) const {
    if (!item || !item->download)
        return QString();

    if (!item->target.isEmpty())
        return item->target;

    const TransferViewItem *group = item->parent();
    if (group && group->download && group->cid.isEmpty() && !group->target.isEmpty())
        return group->target;

    if (item->cid.isEmpty())
        return QString();

    try {
        dcpp::UserPtr user = ClientManager::getInstance()->getUser(CID(_tq(item->cid)));
        if (!user)
            return QString();

        string aTarget;
        int64_t aSize = 0;
        int aFlags = 0;
        if (QueueManager::getInstance()->getQueueInfo(user, aTarget, aSize, aFlags))
            return _q(aTarget);
    }
    catch (const Exception&){}

    return QString();
}

bool TransferView::canRemoveItem(TransferViewItem *item) const {
    if (!item)
        return false;
    if (!item->download)
        return !item->cid.isEmpty();
    return !resolveDownloadTarget(item).isEmpty() || !item->cid.isEmpty();
}

void TransferView::removeDownloadSource(const QString &cid){
    if (cid.isEmpty())
        return;

    removeFromQueue(cid);
    closeConection(cid, true);

    VarMap params;
    params["CID"] = cid;
    params["DOWN"] = true;
    model->removeTransfer(params);
}

void TransferView::removeUploadItem(const QString &cid){
    if (cid.isEmpty())
        return;

    closeConection(cid, false);

    VarMap params;
    params["CID"] = cid;
    params["DOWN"] = false;
    model->removeTransfer(params);
}

void TransferView::removeMenuSelection(const QList<TransferViewItem*> &items){
    QStringList targets;
    QStringList downloadSources;
    QStringList uploadCids;

    for (const auto &i : items) {
        if (i->download) {
            const QString target = resolveDownloadTarget(i);
            if (!target.isEmpty()) {
                if (!targets.contains(target))
                    targets.append(target);
            } else if (!i->cid.isEmpty() && !downloadSources.contains(i->cid)) {
                downloadSources.append(i->cid);
            }
        } else if (!i->cid.isEmpty() && !uploadCids.contains(i->cid)) {
            uploadCids.append(i->cid);
        }
    }

    for (const auto &target : targets)
        removeTransfer(target);
    for (const auto &cid : downloadSources)
        removeDownloadSource(cid);
    for (const auto &cid : uploadCids)
        removeUploadItem(cid);
}
