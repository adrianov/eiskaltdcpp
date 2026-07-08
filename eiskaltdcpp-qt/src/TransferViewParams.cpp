/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferView.h"
#include "TransferViewMetrics.h"
#include "WulforUtil.h"

#include "dcpp/Util.h"
#include "dcpp/Download.h"
#include "dcpp/Upload.h"

using namespace TransferViewMetrics;

void TransferView::getParams(TransferView::VarMap &params, const dcpp::ConnectionQueueItem *item){
    const dcpp::UserPtr &user = item->getUser();
    WulforUtil *WU = WulforUtil::getInstance();

    params["CID"]   = _q(user->getCID().toBase32());
    params["USER"]  = WU->getNicks(user->getCID());
    params["HUB"]   = WU->getHubNames(user);
    params["FAIL"]  = false;
    params["HOST"]  = _q(item->getUser().hint);
    params["DOWN"]  = item->getDownload();
}

void TransferView::getParams(TransferView::VarMap &params, const dcpp::Transfer *trf){
    const UserPtr& user = trf->getUser();
    WulforUtil *WU = WulforUtil::getInstance();

    params["CID"]   = _q(user->getCID().toBase32());

    if (trf->getType() == Transfer::TYPE_PARTIAL_LIST || trf->getType() == Transfer::TYPE_FULL_LIST)
        params["FNAME"] = tr("File list");
    else if (trf->getType() == Transfer::TYPE_TREE)
        params["FNAME"] = QString("TTH: ") + _q(Util::getFileName(trf->getPath()));
    else
        params["FNAME"] = _q(Util::getFileName(trf->getPath()));

    QString nick = WU->getNicks(user->getCID());

    if (!nick.isEmpty())//Do not update user nick if user is offline
        params["USER"]  = nick;

    params["HUB"]   = WU->getHubNames(user);
    params["PATH"]  = _q(Util::getFilePath(trf->getPath()));
    double speed = trf->getUserConnection().getDisplaySpeed();
    if(speed <= 0)
        speed = trf->getAverageSpeed();
    params["SPEED"] = speed;
    params["DPOS"]  = (qlonglong)trf->getPos();

    if (const auto *dl = dynamic_cast<const Download *>(trf)) {
        const DownloadUiState s = downloadState(dl);
        params["ESIZE"] = static_cast<qlonglong>(s.fileSize);
        params["PERC"] = s.fileSize > 0 ? s.downloaded * 100.0 / s.fileSize : 0.0;
    } else {
        params["ESIZE"] = (qlonglong)trf->getSize();
        params["PERC"] = trf->getSize() > 0
            ? static_cast<double>(trf->getPos() * 100.0) / trf->getSize() : 0.0;
    }

    params["IP"]    = _q(trf->getUserConnection().getRemoteIp());
    params["TLEFT"] = qlonglong(trf->getSecondsLeft() > 0 ? trf->getSecondsLeft() : -1);
    params["TARGET"]= _q(trf->getPath());
    params["HOST"]  = _q(trf->getUserConnection().getHubUrl());
    params["DOWN"]  = true;
    params["TTH"] = _q(trf->getTTH().toBase32());
    if (trf->getUserConnection().isSecure())
    {
        params["ENCRYPTION"] = _q(trf->getUserConnection().getCipherName());
    }
    else
    {
        params["ENCRYPTION"] = _q("Plain");
    }
}
