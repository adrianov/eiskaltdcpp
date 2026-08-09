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

#include "SearchModel.h"

#include "dcpp/stdinc.h"
#include "dcpp/ClientManager.h"
#include "dcpp/User.h"

using namespace dcpp;

namespace {

bool cidOffline(const QString &cid)
{
    if (cid.size() != 39)
        return true;
    const UserPtr user = ClientManager::getInstance()->findUser(CID(cid.toStdString()));
    return !user || !user->isOnline();
}

} // namespace

QString SearchModel::dirGroupKey(const QString &path, const QString &file) {
    return path + QLatin1Char('\0') + file;
}

void SearchModel::clearModel(){
    // Notify views/selection before freeing items; deleting first leaves
    // QItemSelection with dangling indexes and crashes in QTreeView paint.
    beginResetModel();
    countSortPending = false;
    rootItem->clearChildren();
    tths.clear();
    dirs.clear();
    hasBitrate_ = hasResolution_ = hasVideo_ = hasAudio_ = false;
    endResetModel();
}

void SearchModel::removeItem(const SearchItem *item){
    if (!okToFind(item))
        return;

    SearchItem *p = const_cast<SearchItem*>(item->parent());
    const int row = item->row();
    const QModelIndex parentIndex = createIndexForItem(p);

    beginRemoveRows(parentIndex, row, row);

    p->removeChild(row);

    if (tths.value(item->data(COLUMN_SF_TTH).toString()) == item)
        tths.remove(item->data(COLUMN_SF_TTH).toString());

    if (item->isDir) {
        const QString key = dirGroupKey(item->data(COLUMN_SF_PATH).toString(),
                                        item->data(COLUMN_SF_FILENAME).toString());
        if (dirs.value(key) == item)
            dirs.remove(key);
    }

    // Flags before endRemoveRows so the parent is not painted with a stale wash.
    if (p != rootItem)
        refreshOfflineTint(p);

    endRemoveRows();

    delete item;

    if (p != rootItem)
        emitGroupDataChanged(p);
}

void SearchModel::setFilterRole(int role){
    filterRole = role;
}

void SearchModel::refreshOfflineTint(SearchItem *item)
{
    if (!item)
        return;

    // Child under a TTH/dir group: update the whole group together.
    SearchItem *group = item;
    if (item->parent() && item->parent()->parent())
        group = item->parent();

    bool allOff = cidOffline(group->cid);
    if (allOff) {
        for (SearchItem *child : group->children()) {
            if (!cidOffline(child->cid)) {
                allOff = false;
                break;
            }
        }
    }

    group->setOfflineTint(allOff);
    for (SearchItem *child : group->children())
        child->setOfflineTint(allOff);
}

void SearchModel::emitGroupDataChanged(SearchItem *group)
{
    const QModelIndex idx = createIndexForItem(group);
    if (!idx.isValid())
        return;
    emit dataChanged(idx, idx.sibling(idx.row(), columnCount() - 1));
    for (SearchItem *child : group->children()) {
        const QModelIndex c = createIndexForItem(child);
        if (c.isValid())
            emit dataChanged(c, c.sibling(c.row(), columnCount() - 1));
    }
}

void SearchModel::emitGroupChanged(SearchItem *group)
{
    refreshOfflineTint(group);
    emitGroupDataChanged(group);
}

void SearchModel::refreshLocal(const QString &tth){
    if (tth.isEmpty()) {
        const QList<QString> keys = tths.keys();
        for (const QString &key : keys)
            refreshLocal(key);
        return;
    }

    SearchItem *item = tths.value(tth);
    if (!item)
        return;

    item->clearLocalPath();
    item->clearQueued();
    for (SearchItem *child : item->children()) {
        child->clearLocalPath();
        child->clearQueued();
    }

    // Local/queue only; presence cache is updated on membership / duplicate SR.
    emitGroupDataChanged(item);
}

bool SearchModel::okToFind(const SearchItem *item){
    if (!item)
        return false;

    if (rootItem->children().contains(const_cast<SearchItem*>(item)))
        return true;

    if (SearchItem *tthRoot = tths.value(item->data(COLUMN_SF_TTH).toString())) {
        for (const auto &i : tthRoot->children()) {
            if (item == i)
                return true;
        }
    }

    if (item->isDir) {
        const QString key = dirGroupKey(item->data(COLUMN_SF_PATH).toString(),
                                        item->data(COLUMN_SF_FILENAME).toString());
        if (SearchItem *dirRoot = dirs.value(key)) {
            for (const auto &i : dirRoot->children()) {
                if (item == i)
                    return true;
            }
        }
    }

    return false;
}
