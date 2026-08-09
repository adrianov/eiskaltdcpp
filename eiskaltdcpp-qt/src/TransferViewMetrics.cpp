/***************************************************************************
*                                                                         *
*   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferViewMetrics.h"

#include "WulforUtil.h"

#include "dcpp/Download.h"
#include "dcpp/File.h"
#include "dcpp/QueueManager.h"
#include "dcpp/Transfer.h"
#include "dcpp/Upload.h"
#include "dcpp/UserConnection.h"
#include "dcpp/Util.h"

#include <QHash>
#include <QObject>

using namespace dcpp;

namespace TransferViewMetrics {

namespace {

static const quint64 DOWNLOAD_UI_INTERVAL_MS = 250;
static QHash<QString, quint64> downloadTickTimes;

int64_t uploadDiskSize(const Upload *ul)
{
    if (ul->getType() != Transfer::TYPE_FILE)
        return -1;
    return File::getSize(ul->getPath());
}

} // namespace

QString downloadTickKey(const Download *dl)
{
    return _q(dl->getUser()->getCID().toBase32()) + QLatin1Char('|') + _q(dl->getPath());
}

bool shouldRefreshDownloadUi(const QString &key)
{
    const quint64 now = GET_TICK();
    const auto it = downloadTickTimes.constFind(key);
    if (it != downloadTickTimes.constEnd() && now - *it < DOWNLOAD_UI_INTERVAL_MS)
        return false;
    downloadTickTimes[key] = now;
    return true;
}

void clearDownloadUiThrottle(const QString &key)
{
    downloadTickTimes.remove(key);
}

void clearDownloadUiThrottleByCid(const QString &cid)
{
    if (cid.isEmpty() || downloadTickTimes.isEmpty())
        return;
    const QString prefix = cid + QLatin1Char('|');
    for (auto it = downloadTickTimes.begin(); it != downloadTickTimes.end(); ) {
        if (it.key().startsWith(prefix))
            it = downloadTickTimes.erase(it);
        else
            ++it;
    }
}

int64_t downloadFileSize(const Transfer *trf)
{
    const int64_t fileSize = QueueManager::getInstance()->getSize(trf->getPath());
    if (fileSize > 0)
        return fileSize;
    return trf->getSize() > 0 ? trf->getSize() : 0;
}

UploadUiState uploadState(const Upload *ul)
{
    UploadUiState s;
    const int64_t startPos = ul->getStartPos();
    const int64_t pos = ul->getPos();
    const int64_t segmentSize = ul->getSize();
    const int64_t diskSize = uploadDiskSize(ul);

    // Progress % uses session transferred bytes in TransferViewModel (not startPos+pos).
    // Here `sent` is this part only; absolute progress is speedBase + Σ parts.
    if (diskSize > 0) {
        s.fileSize = diskSize;
        s.sent = pos;
        if (s.sent > s.fileSize)
            s.sent = s.fileSize;
    } else {
        s.fileSize = segmentSize > 0 ? segmentSize : pos;
        s.sent = pos;
    }
    s.continuing = startPos > 0;
    s.fileDone = diskSize > 0 && startPos + segmentSize >= diskSize;
    return s;
}

DownloadUiState downloadState(const Download *dl)
{
    DownloadUiState s;
    s.fileSize = downloadFileSize(dl);
    s.segmentPos = dl->getPos();
    s.continuing = QueueManager::getInstance()->getPos(dl->getPath()) > 0;
    return s;
}

QString uploadProgressStat(int64_t sent, int64_t fileSize)
{
    const double percent = fileSize > 0 ? qBound(0.0, sent * 100.0 / fileSize, 100.0) : 0.0;
    return QObject::tr("Uploaded %1 (%2%) ").arg(WulforUtil::formatDisplayBytes(sent)).arg(percent, 0, 'f', 1);
}

QString downloadProgressStat(int64_t bytes, int64_t size)
{
    const double percent = size > 0 ? bytes * 100.0 / size : 0.0;
    return QObject::tr("Downloaded %1 (%2%) ").arg(WulforUtil::formatDisplayBytes(bytes)).arg(percent, 0, 'f', 1);
}

QString slotWaitStat(qint64 queuePos, bool trailingSpace)
{
    QString stat = queuePos > 0
        ? QObject::tr("Waiting for slot (#%1)").arg(queuePos)
        : QObject::tr("Waiting for slot");
    if (trailingSpace)
        stat += ' ';
    return stat;
}

void applyUploadMetrics(QVariantMap &params, const UploadUiState &s, const QString &stat)
{
    params["ESIZE"] = static_cast<qlonglong>(s.fileSize);
    // DPOS/PERC come from session progress (bytes transferred / file size).
    params.remove("DPOS");
    params.remove("PERC");
    if (!stat.isEmpty())
        params["STAT"] = stat;
    params["DOWN"] = false;
    params["FAIL"] = false;
}

void applyDownloadMetrics(QVariantMap &params, const DownloadUiState &s, const QString &stat)
{
    // ESIZE = full file. DPOS = in-flight segment; peer totals are summed in the model.
    params["ESIZE"] = static_cast<qlonglong>(s.fileSize);
    params["DPOS"] = static_cast<qlonglong>(s.segmentPos);
    if (!stat.isEmpty())
        params["STAT"] = stat;
    params["DOWN"] = true;
}

void applyUploadSpeed(QVariantMap &params, const Upload *ul, const UploadUiState &)
{
    // Session metrics: transfersession/TransferSessionRate.md. SEGP/BASE = segment.
    params["SEGP"] = static_cast<qlonglong>(ul->getPos());
    params["BASE"] = static_cast<qlonglong>(ul->getStartPos());
    params.remove("SPEED");
    params.remove("TLEFT");
}

void applyDownloadSpeed(QVariantMap &params, const Download *dl, const DownloadUiState &)
{
    // Session metrics: transfersession/TransferSessionRate.md. SEGP = segment bytes.
    params["SEGP"] = static_cast<qlonglong>(dl->getPos());
    params.remove("SPEED");
    params.remove("TLEFT");
}

} // namespace TransferViewMetrics
