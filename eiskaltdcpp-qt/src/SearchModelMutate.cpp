/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SearchModel.h"

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

    endRemoveRows();

    delete item;

    // Parent Count column (unique sources) changed when a grouped child was removed.
    if (p != rootItem && parentIndex.isValid())
        emit dataChanged(parentIndex, parentIndex);
}

void SearchModel::setFilterRole(int role){
    filterRole = role;
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

    const QModelIndex idx = createIndexForItem(item);
    if (idx.isValid())
        emit dataChanged(idx, idx.sibling(idx.row(), columnCount() - 1));

    for (SearchItem *child : item->children()) {
        const QModelIndex c = createIndexForItem(child);
        if (c.isValid())
            emit dataChanged(c, c.sibling(c.row(), columnCount() - 1));
    }
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
