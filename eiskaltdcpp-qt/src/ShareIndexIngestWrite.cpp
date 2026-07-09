/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ShareIndex.h"

#ifdef USE_QT_SQLITE

#include "ShareIndexQueueCore.h"

using namespace ShareIndexWriteQueue;

bool ShareIndex::writeListRows(const QString &cid, const QList<QVariantMap> &rows)
{
    // Large commits (~10s on M1 SQLite+FTS trigram). Write queue is single-threaded;
    // WAL lets searches continue while this transaction runs.
    // Disarm per-INSERT eviction; prune once after the bulk load.
    QSqlDatabase armDb = threadDb();
    if (armDb.isOpen())
        setCapArmed(armDb, false);

    bool loaded = false;
    const int chunk = 100000;
    for (int offset = 0; offset < rows.size(); ) {
        QSqlDatabase db = threadDb();
        if (!db.isOpen()) {
            setLastError(QStringLiteral("threadDb not open"));
            break;
        }

        db.transaction();

        if (offset == 0) {
            QSqlQuery del(db);
            del.prepare("DELETE FROM share_entries WHERE cid = ? AND source = ?");
            del.addBindValue(cid);
            del.addBindValue(int(SourceFileList));
            if (!del.exec()) {
                setLastError(del.lastError().text());
                db.rollback();
                break;
            }
        }

        QSqlQuery ins(db);
        if (!prepareInsert(ins)) {
            setLastError(ins.lastError().text());
            db.rollback();
            break;
        }

        const int end = qMin(offset + chunk, rows.size());
        bool chunkOk = true;
        for (; offset < end; ++offset) {
            if (isStopping()) {
                db.rollback();
                chunkOk = false;
                break;
            }
            if (!bindInsert(ins, rows.at(offset), SourceFileList) || !ins.exec()) {
                setLastError(ins.lastError().text());
                db.rollback();
                chunkOk = false;
                break;
            }
        }
        if (!chunkOk)
            break;

        if (!db.commit()) {
            setLastError(db.lastError().text());
            break;
        }
        if (offset >= rows.size())
            loaded = true;
    }

    armDb = threadDb();
    if (armDb.isOpen()) {
        if (loaded && !isStopping())
            pruneExcess(armDb);
        setCapArmed(armDb, true);
        if (loaded && !isStopping())
            reclaimFreePages(armDb);
    }
    return loaded && !isStopping();
}

#endif
