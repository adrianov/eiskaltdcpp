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
const int kHubQueueCap = 20000;

bool isStopping()
{
    return writeStopping.loadAcquire() != 0;
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

bool takeNextJob(WriteJob &job)
{
    static const WriteKind kOrder[] = { OpenDb, IngestList, UpsertSearch, BumpShowHits };
    for (WriteKind kind : kOrder) {
        for (int i = 0; i < writeQueue.size(); ++i) {
            if (writeQueue.at(i).kind == kind) {
                job = writeQueue.takeAt(i);
                return true;
            }
        }
    }
    if (writeQueue.isEmpty())
        return false;
    job = writeQueue.dequeue();
    return true;
}

} // namespace ShareIndexWriteQueue

#endif
