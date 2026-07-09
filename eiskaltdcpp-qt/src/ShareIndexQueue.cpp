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

void shareIndexRunWriteWorker()
{
    ShareIndex::getInstance()->drainWriteQueue();
}

void ShareIndex::drainWriteQueue()
{
    using namespace ShareIndexWriteQueue;
    for (;;) {
        WriteJob job;
        {
            QMutexLocker lock(&writeMutex);
            if (!takeNextJob(job)) {
                // Enqueue holds the same mutex, so a new job cannot appear here.
                writeWorkerRunning = false;
                disconnectThreadDb();
                return;
            }
        }

        switch (job.kind) {
        case IngestList:
            ingestListSync(job.user, job.listPath, job.hubUrl, job.nick);
            break;
        case IngestCached:
            ingestCachedListsSync();
            break;
        case UpsertSearch:
            upsertFromSearchSync(job.map);
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
    // Current IngestCached is running (not queued); put a fresh one at the end.
    WriteJob job;
    job.kind = IngestCached;
    writeQueue.enqueue(job);
}

#endif
