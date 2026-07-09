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
#include "WulforUtil.h"

#include "dcpp/ClientManager.h"
#include "dcpp/User.h"
#include "dcpp/QueueManager.h"
#include "dcpp/Exception.h"

using namespace dcpp;

QStringList DownloadQueue::getSources(){
    Q_D(DownloadQueue);

    QStringList ret;

    for (const QString &target : d->sources.keys()){
        QString users;

        for (auto it = d->sources[target].begin(); it != d->sources[target].end(); ++it){
            users += it.key() + "(" + it.value() + ") ";
        }

        ret.push_back(target + "::" + users);
    }

    return ret;
}

void DownloadQueue::removeTarget(const QString &target){
    QueueManager *QM = QueueManager::getInstance();

    try {
        QM->remove(target.toStdString());
    }
    catch (const Exception&){}
}

void DownloadQueue::removeSource(const QString &cid, const QString &target){
    QueueManager *QM = QueueManager::getInstance();
    Q_D(DownloadQueue);

    if (d->sources.contains(target) && !cid.isEmpty()){
        UserPtr user = ClientManager::getInstance()->findUser(CID(cid.toStdString()));

        if (user){
            try {
                QM->removeSource(user, QueueItem::Source::FLAG_REMOVED);
            }
            catch (const Exception&){}
        }
    }
}

void DownloadQueue::loadList(){
    VarMap params;

    const QueueItem::StringMap &ll = QueueManager::getInstance()->lockQueue();

    for (const auto &k : ll) {
        getParams(params, k.second);

        addFile(params);
    }

    QueueManager::getInstance()->unlockQueue();

    Q_D(DownloadQueue);

    d->queue_model->sort();
}

void DownloadQueue::addFile(const DownloadQueue::VarMap &map){
    Q_D(DownloadQueue);

    d->queue_model->addItem(map);
    syncSourceMaps(map["TARGET"].toString());
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

    d->queue_model->updItem(map);
    syncSourceMaps(map["TARGET"].toString());
}

QString DownloadQueue::getCID(const VarMap &map){
    if (map.size() < 1)
        return "";

    auto it = map.constBegin();

    return (it.value()).toString();
}

void DownloadQueue::getChilds(DownloadQueueItem *i, QList<DownloadQueueItem *> &list){
    if (!i)
        return;

    if (!i->dir && !list.contains(i)){
        list.push_back(i);

        return;
    }

    if (i->childCount() < 1)
        return;

    for (const auto &ii : i->childItems)
        getChilds(ii, list);
}

void DownloadQueue::getItems(const QModelIndexList &list, QList<DownloadQueueItem*> &items){
    items.clear();

    if (list.isEmpty())
        return;

    for (const auto &i : list){
        DownloadQueueItem *item = reinterpret_cast<DownloadQueueItem*>(i.internalPointer());

        getChilds(item, items);
    }
}

void DownloadQueue::slotCollapseRow(const QModelIndex &row){
    if (row.isValid())
        treeView_TARGET->collapse(row);
}

void DownloadQueue::slotHeaderMenu(const QPoint&){
    WulforUtil::headerMenu(treeView_TARGET);
}

void DownloadQueue::slotUpdateStats(quint64 files, quint64 size){
    if (static_cast<qint64>(size) < 0)
        size = 0;

    if (files == size && size == 0)
        label_STATS->hide();

    if (label_STATS->isHidden())
        label_STATS->show();

    label_STATS->setText(tr("Total files: <b>%1</b> Total size: <b>%2</b>").arg(files).arg(WulforUtil::formatBytes(size)));
}

void DownloadQueue::slotSettingsChanged(const QString &key, const QString &value){
    Q_UNUSED(value)
    if (key == WS_TRANSLATION_FILE)
        retranslateUi(this);
}

