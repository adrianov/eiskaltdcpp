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

namespace ShareIndexWriteQueue {

QMutex writeMutex;
QQueue<WriteJob> writeQueue;
bool writeWorkerRunning = false;
QAtomicInt writeStopping(0);
QAtomicInt abortBackfill(0);
const int kHubQueueCap = 20000;

bool isStopping()
{
    return writeStopping.loadAcquire() != 0;
}

bool shouldAbortBackfill()
{
    return abortBackfill.loadAcquire() != 0 || isStopping();
}

void requestAbortBackfill()
{
    abortBackfill.storeRelease(1);
}

void clearAbortBackfill()
{
    abortBackfill.storeRelease(0);
}

QList<QVariantMap> takeHubUpserts()
{
    QList<QVariantMap> maps;
    for (int i = 0; i < writeQueue.size(); ) {
        if (writeQueue.at(i).kind == UpsertSearch) {
            maps.append(writeQueue.takeAt(i).map);
            continue;
        }
        ++i;
    }
    return maps;
}

void dropOldestHubJob()
{
    for (int i = 0; i < writeQueue.size(); ++i) {
        if (writeQueue.at(i).kind == UpsertSearch) {
            writeQueue.removeAt(i);
            return;
        }
    }
}

static bool takeKind(WriteJob &job, WriteKind kind, bool backfillOnly, bool interactiveOnly)
{
    for (int i = 0; i < writeQueue.size(); ++i) {
        const WriteJob &j = writeQueue.at(i);
        if (j.kind != kind)
            continue;
        if (kind == IngestList) {
            if (interactiveOnly && j.backfill)
                continue;
            if (backfillOnly && !j.backfill)
                continue;
        }
        job = writeQueue.takeAt(i);
        return true;
    }
    return false;
}

bool takeNextJob(WriteJob &job)
{
    // Interactive lists before hub SRs; backfill lists last.
    if (takeKind(job, OpenDb, false, false)
            || takeKind(job, IngestList, false, true)
            || takeKind(job, UpsertSearch, false, false)
            || takeKind(job, BumpShowHits, false, false)
            || takeKind(job, ScanCached, false, false)
            || takeKind(job, IngestList, true, false))
        return true;
    if (writeQueue.isEmpty())
        return false;
    job = writeQueue.dequeue();
    return true;
}

} // namespace ShareIndexWriteQueue

#endif
