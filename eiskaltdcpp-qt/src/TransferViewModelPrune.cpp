/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferViewModel.h"

void TransferViewModel::pruneEmptyParents(){
    for (int row = rootItem->childCount() - 1; row >= 0; --row) {
        TransferViewItem *p = rootItem->child(row);
        if (!p || !p->cid.isEmpty() || p->childCount() > 0)
            continue;
        beginRemoveRows(QModelIndex(), row, row);
        rootItem->childItems.removeAt(row);
        delete p;
        endRemoveRows();
    }
}
