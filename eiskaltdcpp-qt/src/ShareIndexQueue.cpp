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

namespace {

/** Drain queued hub upserts into one SQLite transaction (avoids per-SR commit cost). */
QList<QVariantMap> takeHubUpserts()
{
    using namespace ShareIndexWriteQueue;
    QList<QVariantMap> maps;
    QMutexLocker lock(&writeMutex);
    for (int i = 0; i < writeQueue.size(); ) {
        if (writeQueue.at(i).kind == UpsertSearch) {
            maps.append(writeQueue.takeAt(i).map);
            continue;
        }
        ++i;
    }
    return maps;
}

} // namespace

void shareIndexRunWriteWorker()
{
    ShareIndex::getInstance()->drainWriteQueue();
}

void ShareIndex::drainWriteQueue()
{
    using namespace ShareIndexWriteQueue;
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
            ingestListSync(job.user, job.listPath, job.hubUrl, job.nick);
            break;
        case IngestCached:
            ingestCachedListsSync();
            break;
        case UpsertSearch: {
            QList<QVariantMap> maps;
            maps.append(job.map);
            maps.append(takeHubUpserts());
            upsertFromSearchBatchSync(maps);
            break;
        }
        case BumpShowHits:
            recordSearchShowsSync(job.ids);
            break;
        }
    }
}

bool ShareIndex::pendingListIngest() const
{
    using namespace ShareIndexWriteQueue;
    QMutexLocker lock(&writeMutex);
    for (const WriteJob &job : writeQueue) {
        if (job.kind == IngestList)
            return true;
    }
    return false;
}

void ShareIndex::requeueCachedIngest()
{
    using namespace ShareIndexWriteQueue;
    QMutexLocker lock(&writeMutex);
    for (int i = writeQueue.size() - 1; i >= 0; --i) {
        if (writeQueue.at(i).kind == IngestCached)
            writeQueue.removeAt(i);
    }
    WriteJob job;
    job.kind = IngestCached;
    writeQueue.enqueue(job);
}

#endif
