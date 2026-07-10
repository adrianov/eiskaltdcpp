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
    if (rows.isEmpty()) {
        duckdb::Connection *con = threadConn();
        if (!con) {
            setLastError(QStringLiteral("threadConn not open"));
            return false;
        }
        auto res = ShareIndexDb::query1(
            *con, "DELETE FROM share_entries WHERE cid = ?", ShareIndexDb::strVal(cid));
        if (!res || res->HasError()) {
            setLastError(res ? QString::fromStdString(res->GetError()) : QStringLiteral("delete"));
            return false;
        }
        refreshEntryCount(*con);
        return true;
    }

    bool loaded = false;
    const int chunk = 20000;
    for (int offset = 0; offset < rows.size(); ) {
        if (isStopping())
            break;

        duckdb::Connection *con = threadConn();
        if (!con) {
            setLastError(QStringLiteral("threadConn not open"));
            break;
        }

        if (!ShareIndexDb::execOk(*con, "BEGIN TRANSACTION")) {
            setLastError(QStringLiteral("BEGIN failed"));
            break;
        }

        if (offset == 0) {
            auto del = ShareIndexDb::query1(
                *con, "DELETE FROM share_entries WHERE cid = ?",
                ShareIndexDb::strVal(cid));
            if (!del || del->HasError()) {
                setLastError(del ? QString::fromStdString(del->GetError()) : QStringLiteral("delete"));
                ShareIndexDb::execOk(*con, "ROLLBACK");
                break;
            }
        }

        const int end = qMin(offset + chunk, rows.size());
        QList<QVariantMap> slice;
        slice.reserve(end - offset);
        for (; offset < end; ++offset)
            slice.append(rows.at(offset));

        if (!appendListRows(*con, slice)) {
            ShareIndexDb::execOk(*con, "ROLLBACK");
            break;
        }

        if (!ShareIndexDb::execOk(*con, "COMMIT")) {
            setLastError(QStringLiteral("COMMIT failed"));
            break;
        }

        refreshEntryCount(*con);

        if (offset >= rows.size())
            loaded = true;
    }

    duckdb::Connection *con = threadConn();
    if (con) {
        if (loaded && !isStopping()) {
            refreshEntryCount(*con);
            pruneExcess(*con);
        }
        if (loaded && !isStopping())
            reclaimFreePages(*con);
    }
    return loaded && !isStopping();
}

void ShareIndex::removeTthSync(const QString &cid, const QString &tth)
{
    if (cid.isEmpty() || tth.isEmpty() || isStopping())
        return;
    duckdb::Connection *con = threadConn();
    if (!con) {
        setLastError(QStringLiteral("threadConn not open"));
        return;
    }
    QString err;
    auto res = ShareIndexDb::query2(
        *con,
        "DELETE FROM share_entries WHERE cid = ? AND tth = ?",
        ShareIndexDb::strVal(cid), ShareIndexDb::strVal(tth), &err);
    if (!res || res->HasError()) {
        setLastError(err.isEmpty() && res ? QString::fromStdString(res->GetError()) : err);
        return;
    }
    refreshEntryCount(*con);
}

#endif
