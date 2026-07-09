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
#include "dcpp/QueueItem.h"
#include "dcpp/QueueManager.h"

#include <QDir>

void TransferView::on(dcpp::ConnectionManagerListener::Added, dcpp::ConnectionQueueItem* cqi) noexcept{
    VarMap params;

    getParams(params, cqi);

    params["FNAME"] = "";
    params["STAT"] = tr("Connecting...");

    if(cqi->getDownload()) {
        string aTarget; int64_t aSize; int aFlags = 0;
        if(QueueManager::getInstance()->getQueueInfo(cqi->getUser(), aTarget, aSize, aFlags)) {
            params["TARGET"] = _q(aTarget);
            params["ESIZE"] = (qlonglong)aSize;

            if (!aFlags)
                params["FNAME"] = _q(Util::getFileName(aTarget));

            params["BGROUP"] = !aFlags;
        }
    }

    emit coreCMAdded(params);
}

void TransferView::on(dcpp::ConnectionManagerListener::Connected, dcpp::ConnectionQueueItem* cqi) noexcept{

    VarMap params;

    getParams(params, cqi);

    params["STAT"] = tr("Connected");

    emit coreCMConnected(params);
}

void TransferView::on(dcpp::ConnectionManagerListener::Removed, dcpp::ConnectionQueueItem* cqi) noexcept{
    VarMap params;

    getParams(params, cqi);
    if (cqi->getDownload())
        TransferViewMetrics::clearDownloadUiThrottleByCid(vstr(params["CID"]));

    emit coreCMRemoved(params);
}

void TransferView::on(dcpp::ConnectionManagerListener::Failed, dcpp::ConnectionQueueItem* cqi, const std::string &reason) noexcept{
    VarMap params;

    getParams(params, cqi);

    params["STAT"] = _q(reason);
    params["FAIL"] = true;
    params["SPEED"] = (qlonglong)0;
    params["TLEFT"] = -1;

    emit coreCMFailed(params);
}

void TransferView::on(dcpp::ConnectionManagerListener::StatusChanged, dcpp::ConnectionQueueItem* cqi) noexcept{
    VarMap params;
    getParams(params, cqi);

    if (cqi->getState() == ConnectionQueueItem::CONNECTING)
        params["STAT"] = tr("Connecting");
    else if (cqi->getState() == ConnectionQueueItem::NO_DOWNLOAD_SLOTS)
        params["STAT"] = tr("No download slots");
    else
        params["STAT"] = tr("Waiting to retry");

    emit coreCMStatusChanged(params);
}

void TransferView::on(dcpp::QueueManagerListener::Finished, dcpp::QueueItem* qi, const std::string&, int64_t) noexcept{
    VarMap params;
    params["TARGET"] = _q(qi->getTarget());

    emit coreQMFinished(params);

    if (!qi->isSet(QueueItem::FLAG_USER_LIST))//Do not show notify for filelists
        emit coreDownloadComplete(_q(qi->getTarget()).split(QDir::separator()).last());
}

void TransferView::on(dcpp::QueueManagerListener::Removed, dcpp::QueueItem* qi) noexcept{
    VarMap params;
    params["TARGET"] = _q(qi->getTarget());

    emit coreQMRemoved(params);
}
