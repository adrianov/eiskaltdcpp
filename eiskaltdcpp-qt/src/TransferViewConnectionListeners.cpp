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

namespace {

void fillDownloadTarget(QVariantMap &params, dcpp::ConnectionQueueItem *cqi)
{
    if (!cqi->getDownload())
        return;

    string aTarget;
    int64_t aSize = 0;
    int aFlags = 0;
    if (!QueueManager::getInstance()->getQueueInfo(cqi->getUser(), aTarget, aSize, aFlags))
        return;

    params["TARGET"] = _q(aTarget);
    params["ESIZE"] = static_cast<qlonglong>(aSize);

    if (aFlags & (QueueItem::FLAG_USER_LIST | QueueItem::FLAG_PARTIAL_LIST))
        params["FNAME"] = TransferView::tr("File list");
    else if (!aTarget.empty())
        params["FNAME"] = _q(Util::getFileName(aTarget));
}

} // namespace

void TransferView::on(dcpp::ConnectionManagerListener::Added, dcpp::ConnectionQueueItem* cqi) noexcept{
    VarMap params;

    getParams(params, cqi);

    params["FNAME"] = "";
    params["STAT"] = tr("Connecting...");
    params["SOFT_STAT"] = true;
    fillDownloadTarget(params, cqi);

    emit coreCMAdded(params);
}

void TransferView::on(dcpp::ConnectionManagerListener::Connected, dcpp::ConnectionQueueItem* cqi) noexcept{

    VarMap params;

    getParams(params, cqi);

    params["STAT"] = tr("Connected");
    params["SOFT_STAT"] = true;
    fillDownloadTarget(params, cqi);

    emit coreCMConnected(params);
}

void TransferView::on(dcpp::ConnectionManagerListener::Removed, dcpp::ConnectionQueueItem* cqi) noexcept{
    VarMap params;

    getParams(params, cqi);
    if (cqi->getDownload())
        TransferViewMetrics::clearDownloadUiThrottleByCid(vstr(params["CID"]));
    else
        clearUploadThrottle(vstr(params["CID"]));

    emit coreCMRemoved(params);
}

void TransferView::on(dcpp::ConnectionManagerListener::Failed, dcpp::ConnectionQueueItem* cqi, const std::string &reason) noexcept{
    VarMap params;

    getParams(params, cqi);

    params["STAT"] = _q(reason);
    params["FAIL"] = true;
    params["SPEED"] = (qlonglong)0;
    params["TLEFT"] = -1;
    fillDownloadTarget(params, cqi);

    emit coreCMFailed(params);
}

void TransferView::on(dcpp::ConnectionManagerListener::StatusChanged, dcpp::ConnectionQueueItem* cqi) noexcept{
    VarMap params;
    getParams(params, cqi);

    if (cqi->getState() == ConnectionQueueItem::CONNECTING) {
        params["STAT"] = tr("Connecting");
        params["SOFT_STAT"] = true;
    } else if (cqi->getState() == ConnectionQueueItem::NO_DOWNLOAD_SLOTS)
        params["STAT"] = tr("No download slots");
    else
        params["STAT"] = tr("Waiting to retry");

    fillDownloadTarget(params, cqi);

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
