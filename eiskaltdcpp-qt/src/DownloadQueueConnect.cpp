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

#include "dcpp/QueueManager.h"

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

    applyIndexUsers(rows, true);

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
    applyIndexUsers(row, true);
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

        d->lastIndexAttach.remove(map["TARGET"].toString());
    }
}

void DownloadQueue::updateFile(const DownloadQueue::VarMap &map){
    Q_D(DownloadQueue);

    VarMap row = map;
    // Display + attach only when index shows online holders we have not counted yet.
    // Do not CTM-nudge on every StatusUpdated (that flooded peers and restarted tthl).
    if (row.value("STATUS").toString() != tr("Running..."))
        applyIndexUsers(row, false);
    d->queue_model->updItem(row);
    syncSourceMaps(row["TARGET"].toString());
}
