/***************************************************************************
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

    // Use full-file offsets only when File::getSize succeeds. Falling back to
    // segment size while still adding startPos produced millions of percent.
    if (diskSize > 0) {
        s.fileSize = diskSize;
        s.sent = startPos + pos;
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
    const int64_t base = QueueManager::getInstance()->getPos(dl->getPath());
    s.downloaded = base + dl->getPos();
    s.continuing = base > 0;
    return s;
}

QString uploadProgressStat(int64_t sent, int64_t fileSize)
{
    const double percent = fileSize > 0 ? qBound(0.0, sent * 100.0 / fileSize, 100.0) : 0.0;
    return QObject::tr("Uploaded %1 (%2%) ").arg(WulforUtil::formatDisplayBytes(sent)).arg(percent, 0, 'f', 1);
}

QString downloadProgressStat(int64_t downloaded, int64_t fileSize)
{
    const double percent = fileSize > 0 ? downloaded * 100.0 / fileSize : 0.0;
    return QObject::tr("Downloaded %1 (%2%) ").arg(WulforUtil::formatDisplayBytes(downloaded)).arg(percent, 0, 'f', 1);
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
    params["DPOS"] = static_cast<qlonglong>(s.sent);
    params["PERC"] = s.fileSize > 0
        ? qBound(0.0, s.sent * 100.0 / s.fileSize, 100.0) : 0.0;
    if (!stat.isEmpty())
        params["STAT"] = stat;
    params["DOWN"] = false;
    params["FAIL"] = false;
}

void applyDownloadMetrics(QVariantMap &params, const Download *dl,
                          const DownloadUiState &s, const QString &stat)
{
    params["ESIZE"] = static_cast<qlonglong>(s.fileSize);
    params["DPOS"] = static_cast<qlonglong>(dl->getPos());
    params["PERC"] = s.fileSize > 0 ? s.downloaded * 100.0 / s.fileSize : 0.0;
    if (!stat.isEmpty())
        params["STAT"] = stat;
    params["DOWN"] = true;
}

void applyUploadSpeed(QVariantMap &params, const Upload *ul, const UploadUiState &s)
{
    double speed = ul->getUserConnection().getDisplaySpeed();
    if (speed <= 0)
        speed = ul->getAverageSpeed();
    params["SPEED"] = speed;
    params["TLEFT"] = (speed > 0 && s.fileSize > s.sent)
        ? static_cast<qlonglong>((s.fileSize - s.sent) / speed) : static_cast<qlonglong>(-1);
}

void applyDownloadSpeed(QVariantMap &params, const Download *dl, const DownloadUiState &s)
{
    double speed = dl->getUserConnection().getDisplaySpeed();
    if (speed <= 0)
        speed = dl->getAverageSpeed();
    params["SPEED"] = speed;
    params["TLEFT"] = (speed > 0 && s.fileSize > s.downloaded)
        ? static_cast<qlonglong>((s.fileSize - s.downloaded) / speed) : static_cast<qlonglong>(-1);
}

} // namespace TransferViewMetrics
