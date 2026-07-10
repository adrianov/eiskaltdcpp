/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ShareIndexQueueCore.h"

#ifdef USE_QT_SQLITE

#include <QDir>
#include <QFileInfo>
#include <algorithm>

#include "dcpp/DirectoryListing.h"
#include "dcpp/Util.h"
#include "WulforUtil.h"

using namespace dcpp;
using namespace ShareIndexWriteQueue;

void shareIndexRunWriteWorker()
{
    ShareIndex *idx = ShareIndex::getInstance();
    if (!idx)
        return;
    idx->drainWriteQueue();
}

void ShareIndex::enqueueBackfillList(const UserPtr &user, const QString &listPath,
                                     const QString &nick)
{
    if (!user || listPath.isEmpty())
        return;
    WriteJob job;
    job.kind = IngestList;
    job.user = user;
    job.listPath = listPath;
    job.nick = nick;
    job.backfill = true;
    enqueueWrite(job);
}

void ShareIndex::scanCachedListsSync()
{
    open();
    if (!isOpen())
        return;

    const QString listDir = _q(Util::getListPath());
    QDir dir(listDir);
    if (!dir.exists())
        return;

    // Smallest lists first so the index grows visibly before multi‑GB shares.
    QFileInfoList infos = dir.entryInfoList(QStringList() << "*.xml.bz2" << "*.xml", QDir::Files);
    std::sort(infos.begin(), infos.end(), [](const QFileInfo &a, const QFileInfo &b) {
        return a.size() < b.size();
    });

    for (const QFileInfo &fi : infos) {
        if (isStopping())
            return;

        const QString fullPath = fi.absoluteFilePath();
        const UserPtr user = DirectoryListing::getUserFromFilename(_tq(fullPath));
        if (!user)
            continue;

        const QString cid = _q(user->getCID().toBase32());
        if (!needsListIngest(cid, fullPath))
            continue;

        QString base = fi.fileName();
        if (base.endsWith(QLatin1String(".xml.bz2"), Qt::CaseInsensitive))
            base.chop(8);
        else if (base.endsWith(QLatin1String(".xml"), Qt::CaseInsensitive))
            base.chop(4);
        QString nick;
        const int dot = base.lastIndexOf(QLatin1Char('.'));
        if (dot > 0)
            nick = base.left(dot);

        enqueueBackfillList(user, fullPath, nick);
    }
}

void ShareIndex::drainWriteQueue()
{
    for (;;) {
        if (isStopping()) {
            QMutexLocker lock(&writeMutex);
            writeQueue.clear();
            writeWorkerRunning = false;
            disconnectThreadDb();
            return;
        }

        WriteJob job;
        {
            QMutexLocker lock(&writeMutex);
            if (!takeNextJob(job)) {
                writeWorkerRunning = false;
                disconnectThreadDb();
                return;
            }
        }

        switch (job.kind) {
        case OpenDb:
            open();
            break;
        case IngestList:
            if (!job.backfill)
                clearAbortBackfill();
            ingestListSync(job.user, job.listPath, job.hubUrl, job.nick, false, job.backfill);
            break;
        case ScanCached:
            scanCachedListsSync();
            break;
        case UpsertSearch: {
            QList<QVariantMap> maps;
            maps.append(job.map);
            {
                QMutexLocker lock(&writeMutex);
                maps.append(takeHubUpserts());
            }
            upsertFromSearchBatchSync(maps);
            break;
        }
        case BumpShowHits:
            recordSearchShowsSync(job.ids);
            break;
        }
    }
}

#endif
