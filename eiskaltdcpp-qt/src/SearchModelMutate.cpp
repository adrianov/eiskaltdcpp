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
    blockSignals(true);

    qDeleteAll(rootItem->childItems);
    rootItem->childItems.clear();

    tths.clear();
    dirs.clear();

    blockSignals(false);

    reset();
}

void SearchModel::reset() {
    beginResetModel();
    endResetModel();
}

void SearchModel::removeItem(const SearchItem *item){
    if (!okToFind(item))
        return;

    SearchItem *p = const_cast<SearchItem*>(item->parent());
    const int row = item->row();
    const QModelIndex parentIndex = createIndexForItem(p);

    beginRemoveRows(parentIndex, row, row);

    p->childItems.removeAt(row);

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
}

void SearchModel::setFilterRole(int role){
    filterRole = role;
}

bool SearchModel::okToFind(const SearchItem *item){
    if (!item)
        return false;

    if (rootItem->childItems.contains(const_cast<SearchItem*>(item)))
        return true;

    if (SearchItem *tthRoot = tths.value(item->data(COLUMN_SF_TTH).toString())) {
        for (const auto &i : tthRoot->childItems) {
            if (item == i)
                return true;
        }
    }

    if (item->isDir) {
        const QString key = dirGroupKey(item->data(COLUMN_SF_PATH).toString(),
                                        item->data(COLUMN_SF_FILENAME).toString());
        if (SearchItem *dirRoot = dirs.value(key)) {
            for (const auto &i : dirRoot->childItems) {
                if (item == i)
                    return true;
            }
        }
    }

    return false;
}
