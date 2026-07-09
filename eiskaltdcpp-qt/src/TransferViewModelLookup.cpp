/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferViewModel.h"

bool TransferViewModel::findTransfer(const QString &cid, bool download, TransferViewItem **item){
    if (!item)
        return false;

    auto i = transfer_hash.find(cid);
    while (i != transfer_hash.end() && i.key() == cid && !cid.isEmpty()){
        if (i.value()->download == download){
           *item = i.value();
           return true;
        }
        ++i;
    }
    return false;
}

bool TransferViewModel::findParent(const QString &target, TransferViewItem **item, bool download){
    if (!item)
        return false;

    for (const auto &i : rootItem->childItems){
        if ((i->download == download) && i->target == target && i->cid.isEmpty()){
            *item = i;
            return true;
        }
    }
    return false;
}

TransferViewItem *TransferViewModel::getParent(const QString &target, const VarMap &params){
    TransferViewItem *p = nullptr;
    if (findParent(target, &p))
        return p;

    QList<QVariant> data;
    data << params["USER"] << 0 << "" << "" << params["ESIZE"]
         << params["TLEFT"] << params["FNAME"] << params["HOST"] << "" << "";

    p = new TransferViewItem(data, rootItem);
    p->download = true;
    p->target = target;
    p->dpos = params.value("FPOS").toLongLong();
    rootItem->appendChild(p);
    return p;
}
