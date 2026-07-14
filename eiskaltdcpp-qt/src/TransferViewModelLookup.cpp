/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferViewModel.h"

bool TransferViewModel::findTransfer(const QString &cid, bool download, TransferViewItem **item,
                                     const QString &hub){
    if (!item)
        return false;

    auto i = transfer_hash.find(cid);
    while (i != transfer_hash.end() && i.key() == cid && !cid.isEmpty()){
        TransferViewItem *row = i.value();
        if (row->download == download
                && (download || hub.isEmpty()
                    || vstr(row->data(COLUMN_TRANSFER_HOST)) == hub)){
           *item = row;
           return true;
        }
        ++i;
    }
    return false;
}

bool TransferViewModel::findParent(const QString &target, TransferViewItem **item, bool download,
                                   const QString &ip){
    if (!item || target.isEmpty())
        return false;
    if (!download && ip.isEmpty())
        return false;

    for (const auto &i : rootItem->childItems){
        if (i->download != download || i->target != target || !i->cid.isEmpty())
            continue;
        if (!download && vstr(i->data(COLUMN_TRANSFER_IP)) != ip)
            continue;
        *item = i;
        return true;
    }
    return false;
}

TransferViewItem *TransferViewModel::getParent(const QString &target, const VarMap &params){
    const bool download = vbol(params["DOWN"]);
    const QString ip = vstr(params["IP"]);
    TransferViewItem *p = nullptr;
    if (findParent(target, &p, download, ip))
        return p;

    QList<QVariant> data;
    data << params["USER"] << 0 << "" << "" << params["ESIZE"]
         << params["TLEFT"] << params["FNAME"] << params["HOST"] << params["TAG"]
         << (download ? QVariant("") : params["IP"]) << "";

    p = new TransferViewItem(data, rootItem);
    p->download = download;
    p->target = target;
    p->fpos = params.value("FPOS").toLongLong();
    p->dpos = p->fpos;
    rootItem->appendChild(p);
    return p;
}
