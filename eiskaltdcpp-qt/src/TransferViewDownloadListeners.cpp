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

#include "dcpp/Download.h"
#include "dcpp/QueueManager.h"
#include "extra/ipfilter.h"

#include <QHash>

using namespace TransferViewMetrics;

namespace {

static const quint64 DOWNLOAD_UI_INTERVAL_MS = 250;
static QHash<QString, quint64> downloadTickTimes;

QString downloadTickKey(const dcpp::Download *dl) {
    return _q(dl->getUser()->getCID().toBase32()) + QLatin1Char('|') + _q(dl->getPath());
}

bool shouldRefreshDownloadUi(const QString &key) {
    const quint64 now = GET_TICK();
    const auto it = downloadTickTimes.constFind(key);
    if (it != downloadTickTimes.constEnd() && now - *it < DOWNLOAD_UI_INTERVAL_MS)
        return false;
    downloadTickTimes[key] = now;
    return true;
}

void clearDownloadUiThrottle(const QString &key) {
    downloadTickTimes.remove(key);
}

} // namespace

void TransferView::on(dcpp::DownloadManagerListener::Requesting, dcpp::Download* dl) noexcept{
    VarMap params;

    getParams(params, dl);

    if (IPFilter::getInstance()){
        if (!IPFilter::getInstance()->OK(vstr(params["IP"]).toStdString(), eDIRECTION_IN)){
            closeConection(vstr(params["CID"]), true);
            return;
        }
    }

    params["FPOS"]  = (qlonglong)QueueManager::getInstance()->getPos(dl->getPath());
    const DownloadUiState s = downloadState(dl);
    applyDownloadMetrics(params, dl, s, tr("Requesting"));
    params["FAIL"]  = false;

    emit coreDMRequesting(params);
}

void TransferView::on(dcpp::DownloadManagerListener::Queued, dcpp::Download* dl, size_t queuePos) noexcept{
    VarMap params;

    getParams(params, dl);

    params["QUEUE_POS"] = static_cast<qlonglong>(queuePos);
    params["STAT"] = slotWaitStat(static_cast<qint64>(queuePos));
    params["FAIL"] = false;

    emit coreDMQueued(params);
}

void TransferView::on(dcpp::DownloadManagerListener::Starting, dcpp::Download* dl) noexcept{
    VarMap params;

    getParams(params, dl);

    params["FPOS"]  = (qlonglong)QueueManager::getInstance()->getPos(dl->getPath());
    params["QUEUE_POS"] = static_cast<qlonglong>(0);
    const DownloadUiState s = downloadState(dl);
    const QString stat = s.continuing ? downloadProgressStat(s.downloaded, s.fileSize) : tr("Download starting...");
    applyDownloadMetrics(params, dl, s, stat);
    applyDownloadSpeed(params, dl, s);

    emit coreDMStarting(params);
}

void TransferView::on(dcpp::DownloadManagerListener::Tick, const dcpp::DownloadList& dls) noexcept{
    bool any = false;
    for (const auto &it : dls){
        Download* dl = it;
        const QString tickKey = downloadTickKey(dl);
        if (!shouldRefreshDownloadUi(tickKey))
            continue;

        VarMap params;
        QString str;

        getParams(params, dl);
        params["FPOS"]  = (qlonglong)QueueManager::getInstance()->getPos(dl->getPath());
        const DownloadUiState s = downloadState(dl);

        applyDownloadMetrics(params, dl, s, QString());
        applyDownloadSpeed(params, dl, s);

        if (dl->getUserConnection().isSecure())
        {
            if (dl->getUserConnection().isTrusted())
               str += QString("[S]");
            else
               str += QString("[U]");
        }

        if (dl->isSet(Download::FLAG_TTH_CHECK))
            str += QString("[T]");
        if (dl->isSet(Download::FLAG_ZDOWNLOAD))
            str += QString("[Z]");
        
        params["FLAGS"] = str;
        params["STAT"] = downloadProgressStat(s.downloaded, s.fileSize);

        emit coreDMTick(params);
        any = true;
    }

    if (any)
        emit coreUpdateParents();
}

void TransferView::on(dcpp::DownloadManagerListener::Complete, dcpp::Download* dl) noexcept{
    clearDownloadUiThrottle(downloadTickKey(dl));

    VarMap params;

    getParams(params, dl);

    const DownloadUiState s = downloadState(dl);
    applyDownloadMetrics(params, dl, s, tr("Download complete"));
    params["SPEED"] = 0;

    qint64 pos = QueueManager::getInstance()->getPos(dl->getPath()) + dl->getPos();

    emit coreDMComplete(params);
    emit coreUpdateTransferPosition(params, pos);
}

void TransferView::on(dcpp::DownloadManagerListener::Failed, dcpp::Download* dl, const std::string& reason) noexcept {
    onFailed(dl, reason);
}

void TransferView::on(dcpp::QueueManagerListener::CRCFailed, dcpp::Download* dl, const std::string& reason) noexcept {
    onFailed(dl, reason);
}

void TransferView::onFailed(dcpp::Download* dl, const std::string& reason) {
    clearDownloadUiThrottle(downloadTickKey(dl));

    VarMap params;

    getParams(params, dl);

    const DownloadUiState s = downloadState(dl);
    applyDownloadMetrics(params, dl, s, _q(reason));
    params["SPEED"] = 0;
    params["FAIL"]  = true;
    params["TLEFT"] = -1;

    qint64 pos = QueueManager::getInstance()->getPos(dl->getPath()) + dl->getPos();

    emit coreDMFailed(params);
    emit coreUpdateTransferPosition(params, pos);
}
