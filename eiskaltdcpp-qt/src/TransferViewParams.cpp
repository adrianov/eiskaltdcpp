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

#include "dcpp/ClientManager.h"
#include "dcpp/Util.h"
#include "dcpp/Download.h"

using namespace TransferViewMetrics;

namespace {

/** getNicks can return "" when an online identity has a blank NI. */
QString transferUserNick(const dcpp::UserPtr &user, const std::string &hub)
{
    if (!user)
        return QString();
    const QString nick = WulforUtil::getInstance()->getNicks(user->getCID(), _q(hub));
    return nick.isEmpty() ? '{' + _q(user->getCID().toBase32()) + '}' : nick;
}

QString transferUserTag(const dcpp::UserPtr &user, const std::string &hub)
{
    if (!user)
        return QString();
    // Prefer hub hint; fall back to any online identity for the CID.
    if (OnlineUser *ou = ClientManager::getInstance()->findOnlineUser(user->getCID(), hub, false))
        return _q(ou->getIdentity().getTag());
    return QString();
}

} // namespace

void TransferView::getParams(TransferView::VarMap &params, const dcpp::ConnectionQueueItem *item){
    const dcpp::UserPtr &user = item->getUser();
    WulforUtil *WU = WulforUtil::getInstance();
    const std::string host = ClientManager::getInstance()->resolveHubHint(user, item->getUser().hint);

    params["CID"]   = _q(user->getCID().toBase32());
    params["USER"]  = transferUserNick(user, host);
    params["HUB"]   = WU->getHubNames(user);
    params["FAIL"]  = false;
    params["HOST"]  = host.empty() ? QString() : _q(host);
    params["TAG"]   = transferUserTag(user, host);
    params["DOWN"]  = item->getDownload();
}

void TransferView::getParams(TransferView::VarMap &params, const dcpp::Transfer *trf){
    const UserPtr& user = trf->getUser();
    WulforUtil *WU = WulforUtil::getInstance();
    const std::string host = ClientManager::getInstance()->resolveHubHint(
            user, trf->getUserConnection().getHubUrl());

    params["CID"]   = _q(user->getCID().toBase32());
    params["USER"]  = transferUserNick(user, host);

    if (trf->getType() == Transfer::TYPE_PARTIAL_LIST || trf->getType() == Transfer::TYPE_FULL_LIST)
        params["FNAME"] = tr("File list");
    else if (trf->getType() == Transfer::TYPE_TREE)
        params["FNAME"] = QString("TTH: ") + _q(Util::getFileName(trf->getPath()));
    else
        params["FNAME"] = _q(Util::getFileName(trf->getPath()));

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
    params["HOST"]  = host.empty() ? QString() : _q(host);
    params["TAG"]   = transferUserTag(user, host);
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
