/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "DownloadQueueModel.h"
#include "DownloadQueueModelPrivate.h"
#include "WulforUtil.h"

bool DownloadQueueModel::remItem(const QVariantMap &map){
    DownloadQueueItem *item = createPath(map["PATH"].toString());

    if (item->childItems.size() < 1)
        return false;

    QString target_name = map["FNAME"].toString();
    DownloadQueueItem *target = findTarget(item, target_name);

    if (!target)
        return false;

    Q_D(static DownloadQueueModel);

    d->total_size -= target->data(COLUMN_DOWNLOADQUEUE_ESIZE).toULongLong();
    d->total_files--;

    if (item->childCount() > 1){
        beginRemoveRows(createIndexForItem(item), target->row(), target->row());
        {
            int r = target->row();

            item->childItems.removeAt(r);

            delete target;
        }
        endRemoveRows();
    }
    else {

        DownloadQueueItem *p = item;
        DownloadQueueItem *_t = nullptr;

        while (true) {
            if ((p == d->rootItem) || (p->childCount() > 1) || !p->parent())
                break;

            beginRemoveRows(createIndexForItem(p->parent()), p->row(), p->row());
            {
                p->parent()->childItems.removeAt(p->row());

                _t = p;
            }
            endRemoveRows();

            if (p->parent()->childCount() > 0)
                break;

            p = p->parent();

            delete _t;
        }
    }

    emit updateStats(d->total_files, d->total_size);

    return true;
}

DownloadQueueItem *DownloadQueueModel::createPath(const QString & path){
    Q_D(static DownloadQueueModel);

    if (!d->rootItem)
        return nullptr;

    QString _path = path;
    _path.replace("\\", "/");

    QStringList list = _path.split("/", WULFOR_SKIP_EMPTY);

    DownloadQueueItem *root = d->rootItem;

    bool found = false;

    for (int i = 0; i < list.size(); i++){
        found = false;

        for (const auto &item : root->childItems){
            if (!item->dir)
                continue;

            QString name = item->data(COLUMN_DOWNLOADQUEUE_NAME).toString();

            if (name == list.at(i)){
                found = true;
                root = item;

                break;
            }
        }

        if (!found){
            static QString data = "";

            for (int j = i; j < list.size(); j++){
                QList<QVariant> rootData;
                rootData << list.at(j)  << data << data << data
                         << data << data << data << data
                         << data << data << data;

                DownloadQueueItem *item = new DownloadQueueItem(rootData);
                item->dir = true;

                const QModelIndex parentIdx = createIndexForItem(root);
                beginInsertRows(parentIdx, root->childCount(), root->childCount());
                root->appendChild(item);
                endInsertRows();

                root = item;
            }

            return root;
        }
    }

    return root;
}

DownloadQueueItem *DownloadQueueModel::findTarget(const DownloadQueueItem *item, const QString &name){
    DownloadQueueItem *target = nullptr;

    for (const auto &i : item->childItems){
        if (i->data(COLUMN_DOWNLOADQUEUE_NAME).toString() == name){
            target = i;

            break;
        }
    }

    return target;
}

void DownloadQueueModel::clear(){
    blockSignals(true);

    Q_D(DownloadQueueModel);

    qDeleteAll(d->rootItem->childItems);
    d->rootItem->childItems.clear();

    blockSignals(false);

    emit layoutChanged();
}

void DownloadQueueModel::repaint(){
    emit layoutChanged();
}
