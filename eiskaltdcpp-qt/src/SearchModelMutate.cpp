/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SearchModel.h"

void SearchModel::clearModel(){
    blockSignals(true);

    qDeleteAll(rootItem->childItems);
    rootItem->childItems.clear();

    tths.clear();

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

    if (tths[item->data(COLUMN_SF_TTH).toString()] == item)
        tths.remove(item->data(COLUMN_SF_TTH).toString());

    endRemoveRows();

    delete item;
}

void SearchModel::setFilterRole(int role){
    filterRole = role;
}

bool SearchModel::okToFind(const SearchItem *item){
    if (!item)
        return false;

    if (!rootItem->childItems.contains(const_cast<SearchItem*>(item))){
        QString tth = item->data(COLUMN_SF_TTH).toString();

        SearchItem *tth_root = tths.value(tth);//try to find item by tth

        for (const auto &i : tth_root->childItems){
            if (item == i)
                return true;
        }
    }
    else {
        return true;
    }

    return false;
}

