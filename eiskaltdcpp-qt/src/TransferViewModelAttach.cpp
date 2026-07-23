/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferViewModel.h"
#include "TransferViewModelTree.h"
#include "TransferViewRemoveUtil.h"

void TransferViewModel::initTransfer(const VarMap &params){
    if (params.empty())
        return;

    const QString hub = vbol(params["DOWN"]) ? QString() : vstr(params["HOST"]);
    TransferViewItem *item = nullptr;
    if (!findTransfer(vstr(params["CID"]), vbol(params["DOWN"]), &item, hub)) {
        addConnection(params);
        if (!findTransfer(vstr(params["CID"]), vbol(params["DOWN"]), &item, hub))
            return;
    }

    updateTransfer(params);
}

void TransferViewModel::addConnection(const VarMap &params){
    if (params.empty())
        return;

    if (TransferViewRemove::offlineOrphan(vstr(params["CID"]), vstr(params["HOST"])))
        return;

    const QString hub = vbol(params["DOWN"]) ? QString() : vstr(params["HOST"]);
    TransferViewItem *existing = nullptr;
    if (findTransfer(vstr(params["CID"]), vbol(params["DOWN"]), &existing, hub))
        return;

    const bool bDownload = vbol(params["DOWN"]);
    const QString fname = vstr(params["FNAME"]);
    // Defer tree insert for empty/hidden names when filtering; attach once FNAME is known.
    // Do not create a file parent yet — that would leave an empty orphan row.
    const bool deferAttach = showTranferedFilesOnly && TransferViewTree::isHiddenName(fname);
    const bool bGroup = !deferAttach && TransferViewTree::wantsParent(params, fname);
    TransferViewItem *to = bGroup ? getParent(vstr(params["TARGET"]), params) : nullptr;
    TransferViewItem *parent = (to && bGroup) ? to : rootItem;

    QList<QVariant> data;
    data << params["USER"] << "" << params["STAT"] << "" << "" << ""
         << params["FNAME"] << params["HOST"] << params["TAG"]
         << params.value("IP") << "";

    TransferViewItem *item = new TransferViewItem(data, parent);
    item->download = bDownload;
    item->cid = vstr(params["CID"]);
    item->target = vstr(params["TARGET"]);

    transfer_hash.insert(item->cid, item);

    if (deferAttach)
        return;

    TransferViewTree::attach(item, parent);
    emit layoutChanged();
}
