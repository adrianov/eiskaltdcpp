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

        try {
            switch (job.kind) {
            case OpenDb:
                open();
                break;
            case MatchQueue: {
                dcpp::UserList users = job.users;
                {
                    QMutexLocker lock(&writeMutex);
                    const dcpp::UserList pending = takeMatchUsers();
                    users.insert(users.end(), pending.begin(), pending.end());
                }
                matchQueueSync(users);
                break;
            }
            case RemoveTth:
                removeTthSync(job.cid, job.tth);
                break;
            case RemoveUser:
                removeUserSync(job.cid);
                break;
            case IngestList:
                ingestListSync(job.user, job.listPath, job.hubUrl, job.nick);
                break;
            case UpsertMedia:
                upsertMediaSync(job.media);
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
            }

            // Open already erases+retries once on a poisoned DB; avoid a second rebuild loop.
            if (ShareIndexDb::takeFatal() && !(job.kind == OpenDb && isOpen()))
                recoverDb();

            maybeRunWriteTail();
        } catch (const duckdb::FatalException &e) {
            const QString msg = QString::fromUtf8(e.what());
            ShareIndexDb::noteFailure(duckdb::ExceptionType::FATAL, msg);
            setLastError(msg);
        } catch (const duckdb::InternalException &e) {
            const QString msg = QString::fromUtf8(e.what());
            ShareIndexDb::noteFailure(duckdb::ExceptionType::INTERNAL, msg);
            setLastError(msg);
        } catch (const std::exception &e) {
            const QString msg = QString::fromUtf8(e.what());
            ShareIndexDb::noteFailure(duckdb::ErrorData(e).Type(), msg);
            setLastError(msg);
        } catch (...) {
            setLastError(QStringLiteral("share index write failed"));
        }

        // Maintenance may also invalidate the DB; heal before the next job or exit.
        if (ShareIndexDb::takeFatal())
            recoverDb();
    }
}

bool ShareIndex::runWriteTail(duckdb::Connection &con)
{
    if (writeTailSweep && !removeOrphans(con))
        return false;
    writeTailSweep = false;
    if (!refreshEntryCount(con))
        return false;
    // Fold the WAL back into the DB; skip on shutdown so the worker exits
    // promptly and DuckDB replays the WAL on the next open.
    if (isStopping())
        return true;
    QString err;
    if (!ShareIndexDb::execOk(con, "CHECKPOINT", &err)) {
        setLastError(err);
        return false;
    }
    writeTailCount = false;
    return true;
}

void ShareIndex::maybeRunWriteTail()
{
    if (!writeTailCount)
        return;
    bool drained = false;
    {
        QMutexLocker lock(&writeMutex);
        drained = writeQueue.isEmpty();
    }
    if (!drained && writeTailClock.isValid()
            && writeTailClock.elapsed() < kWriteTailIntervalMs)
        return;
    duckdb::Connection *con = threadConn();
    if (!con)
        return; // no connection yet: keep the request pending for the next boundary
    writeTailClock.start();
    // runWriteTail consumes the flags on success only; a failure or throw
    // leaves them pending for the next maintenance boundary.
    runWriteTail(*con);
}

#endif
