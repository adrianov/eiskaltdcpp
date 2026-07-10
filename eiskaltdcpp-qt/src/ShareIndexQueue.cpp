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

using namespace ShareIndexWriteQueue;

void shareIndexRunWriteWorker()
{
    ShareIndex *idx = ShareIndex::getInstance();
    if (!idx)
        return;
    idx->drainWriteQueue();
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
            ingestListSync(job.user, job.listPath, job.hubUrl, job.nick);
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
