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

#include <QObject>

using namespace dcpp;

namespace TransferViewMetrics {

namespace {

int64_t uploadFileSize(const Upload *ul)
{
    if (ul->getType() != Transfer::TYPE_FILE)
        return ul->getSize();
    const int64_t size = File::getSize(ul->getPath());
    return size > 0 ? size : ul->getSize();
}

} // namespace

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
    s.fileSize = uploadFileSize(ul);
    s.sent = ul->getStartPos() + ul->getPos();
    s.continuing = ul->getStartPos() > 0;
    s.fileDone = s.fileSize > 0 && ul->getStartPos() + ul->getSize() >= s.fileSize;
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
    const double percent = fileSize > 0 ? sent * 100.0 / fileSize : 0.0;
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
    params["PERC"] = s.fileSize > 0 ? s.sent * 100.0 / s.fileSize : 0.0;
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
