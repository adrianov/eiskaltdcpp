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

#include "dcpp/Upload.h"
#include "extra/ipfilter.h"

#include <QHash>

using namespace TransferViewMetrics;

namespace {

static const quint64 UPLOAD_UI_INTERVAL_MS = 250;
static QHash<QString, quint64> uploadTickTimes;

QString uploadTickKey(const dcpp::Upload *ul) {
    return _q(ul->getUser()->getCID().toBase32()) + QLatin1Char('|') + _q(ul->getPath());
}

bool shouldRefreshUploadUi(const QString &key) {
    const quint64 now = GET_TICK();
    const auto it = uploadTickTimes.constFind(key);
    if (it != uploadTickTimes.constEnd() && now - *it < UPLOAD_UI_INTERVAL_MS)
        return false;
    uploadTickTimes[key] = now;
    return true;
}

void clearUploadUiThrottle(const QString &key) {
    uploadTickTimes.remove(key);
}

void clearUploadUiThrottleByCid(const QString &cid) {
    if (cid.isEmpty() || uploadTickTimes.isEmpty())
        return;
    const QString prefix = cid + QLatin1Char('|');
    for (auto it = uploadTickTimes.begin(); it != uploadTickTimes.end(); ) {
        if (it.key().startsWith(prefix))
            it = uploadTickTimes.erase(it);
        else
            ++it;
    }
}

} // namespace

void TransferView::clearUploadThrottle(const QString &cid) {
    clearUploadUiThrottleByCid(cid);
}

void TransferView::on(dcpp::UploadManagerListener::Starting, dcpp::Upload* ul) noexcept{
    VarMap params;
    getParams(params, ul);
    const UploadUiState s = uploadState(ul);
    const QString stat = s.continuing ? uploadProgressStat(s.sent, s.fileSize) : tr("Upload starting...");
    applyUploadMetrics(params, s, stat);
    applyUploadSpeed(params, ul, s);

    if (IPFilter::getInstance()){
        if (!IPFilter::getInstance()->OK(vstr(params["IP"]).toStdString(), eDIRECTION_OUT)){
            closeConection(vstr(params["CID"]), false);
            return;
        }
    }

    emit coreUMStarting(params);
}

void TransferView::on(dcpp::UploadManagerListener::Tick, const dcpp::UploadList& uls) noexcept{
    for (const auto &it : uls){
        Upload* ul = it;
        const QString tickKey = uploadTickKey(ul);
        if (!shouldRefreshUploadUi(tickKey))
            continue;

        VarMap params;
        getParams(params, ul);
        const UploadUiState s = uploadState(ul);
        QString stat;

        applyUploadMetrics(params, s, QString());
        applyUploadSpeed(params, ul, s);

        if (ul->getUserConnection().isSecure())
        {
            if (ul->getUserConnection().isTrusted())
                stat += QString("[S]");
            else
                stat += QString("[U]");
        }
        if (ul->isSet(dcpp::Upload::FLAG_ZUPLOAD))
            stat += QString("[Z]");
        
        params["FLAGS"] = stat;
        params["STAT"] = uploadProgressStat(s.sent, s.fileSize);

        emit coreUMTick(params);
    }
}

void TransferView::on(dcpp::UploadManagerListener::Complete, dcpp::Upload* ul) noexcept{
    VarMap params;
    getParams(params, ul);
    const UploadUiState s = uploadState(ul);
    const QString tickKey = uploadTickKey(ul);

    if (!s.fileDone) {
        applyUploadMetrics(params, s, uploadProgressStat(s.sent, s.fileSize));
        applyUploadSpeed(params, ul, s);
        emit coreUMTick(params);
        return;
    }

    clearUploadUiThrottle(tickKey);
    applyUploadMetrics(params, s, tr("Upload complete"));
    applyUploadSpeed(params, ul, s);
    params["SPEED"] = 0.0;
    params["TLEFT"] = qlonglong(-1);
    params["DOWN"] = false;
    params["FAIL"] = false;

    emit coreUMComplete(params);
}
