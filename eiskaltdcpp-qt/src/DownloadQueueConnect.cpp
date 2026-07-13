/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "DownloadQueue.h"
#include "DownloadQueuePrivate.h"
#include "DownloadQueueModel.h"

#include "dcpp/ClientManager.h"
#include "dcpp/ConnectionManager.h"
#include "dcpp/QueueManager.h"
#include "dcpp/User.h"
#include "dcpp/Util.h"

using namespace dcpp;

void DownloadQueue::loadList(){
    QList<VarMap> rows;

    const QueueItem::StringMap &ll = QueueManager::getInstance()->lockQueue();
    for (const auto &k : ll) {
        VarMap params;
        getParams(params, k.second);
        rows.append(params);
    }
    QueueManager::getInstance()->unlockQueue();

    applyIndexUsers(rows);

    Q_D(DownloadQueue);
    for (const VarMap &params : rows) {
        d->queue_model->addItem(params);
        syncSourceMaps(params["TARGET"].toString());
    }
    d->queue_model->sort();
}

void DownloadQueue::addFile(const DownloadQueue::VarMap &map){
    Q_D(DownloadQueue);

    VarMap row = map;
    applyIndexUsers(row);
    d->queue_model->addItem(row);
    syncSourceMaps(row["TARGET"].toString());
}

void DownloadQueue::remFile(const VarMap &map){
    Q_D(DownloadQueue);

    if (d->queue_model->remItem(map)){
        auto it = d->sources.find(map["TARGET"].toString());

        if (it != d->sources.end())
            d->sources.erase(it);

        it = d->badSources.find(map["TARGET"].toString());

        if (it != d->badSources.end())
            d->badSources.erase(it);
    }
}

void DownloadQueue::updateFile(const DownloadQueue::VarMap &map){
    Q_D(DownloadQueue);

    VarMap row = map;
    if (row.value("STATUS").toString() != tr("Running...")) {
        // Always rematch online index holders (adds missing sources; nudges existing ones).
        applyIndexUsers(row, true);
        d->queue_model->updItem(row);
        syncSourceMaps(row["TARGET"].toString());
        nudgeSourceConnects(row["TARGET"].toString());
        return;
    }
    d->queue_model->updItem(row);
    syncSourceMaps(row["TARGET"].toString());
}

void DownloadQueue::nudgeSourceConnects(const QString &target)
{
    if (target.isEmpty())
        return;

    Q_D(DownloadQueue);
    if (!d->sources.contains(target))
        return;

    QueueManager *qm = QueueManager::getInstance();
    ConnectionManager *cm = ConnectionManager::getInstance();
    ClientManager *cl = ClientManager::getInstance();
    for (auto it = d->sources[target].constBegin(); it != d->sources[target].constEnd(); ++it) {
        const QString &cid = it.value();
        if (cid.size() != 39)
            continue;
        UserPtr user = cl->findUser(CID(cid.toStdString()));
        if (!user || !user->isOnline())
            continue;
        if (qm->hasDownload(user) == QueueItem::PAUSED)
            continue;
        string hint;
        const StringList hubs = cl->getHubs(user->getCID(), Util::emptyString);
        if (!hubs.empty())
            hint = hubs.front();
        cm->getDownloadConnection(HintedUser(user, hint));
    }
}
