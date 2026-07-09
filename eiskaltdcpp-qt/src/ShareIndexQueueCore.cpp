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
const int kHubQueueCap = 2000;

bool takeNextJob(WriteJob &job)
{
    for (int i = 0; i < writeQueue.size(); ++i) {
        if (writeQueue.at(i).kind != UpsertSearch) {
            job = writeQueue.takeAt(i);
            return true;
        }
    }
    if (writeQueue.isEmpty())
        return false;
    job = writeQueue.dequeue();
    return true;
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

} // namespace ShareIndexWriteQueue

#endif
