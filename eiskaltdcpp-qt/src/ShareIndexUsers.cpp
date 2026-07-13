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

#include <QSet>

namespace {

std::string bindMarks(size_t count)
{
    std::string sql;
    for (size_t i = 0; i < count; ++i)
        sql += i ? ",?" : "?";
    return sql;
}

} // namespace

QHash<QString, QList<ShareIndex::IndexUser>>
ShareIndex::usersByTth(const QStringList &tths, qint64 size, int limitPerTth)
{
    QHash<QString, QList<IndexUser>> out;
    if (tths.isEmpty() || !isOpen() || limitPerTth <= 0)
        return out;

    open();
    duckdb::Connection *con = threadConn();
    if (!con)
        return out;

    QStringList unique;
    QSet<QString> seen;
    for (const QString &t : tths) {
        if (t.isEmpty() || seen.contains(t))
            continue;
        seen.insert(t);
        unique << t;
    }
    if (unique.isEmpty())
        return out;

    const size_t n = size_t(unique.size());
    std::string sql =
        "SELECT f.tth,u.nick,u.cid,f.size FROM share_locations l "
        "JOIN share_users u ON u.user_id=l.user_id "
        "JOIN share_files f ON f.file_id=l.file_id "
        "WHERE l.is_dir=0 AND f.tth IN (" + bindMarks(n) + ")";
    duckdb::vector<duckdb::Value> binds;
    for (const QString &t : unique)
        binds.push_back(ShareIndexDb::strVal(t));
    if (size > 0) {
        sql += " AND f.size=?";
        binds.push_back(ShareIndexDb::i64Val(size));
    }
    sql += " ORDER BY f.tth,u.nick";

    QString error;
    auto result = ShareIndexDb::queryMat(*con, sql, binds, &error);
    if (!result) {
        setLastError(error);
        return out;
    }
    setLastError(QString());

    QHash<QString, QSet<QString>> cids;
    for (idx_t row = 0; row < result->RowCount(); ++row) {
        const QString tth = ShareIndexDb::qstr(result->GetValue(0, row));
        const QString nick = ShareIndexDb::qstr(result->GetValue(1, row));
        const QString cid = ShareIndexDb::qstr(result->GetValue(2, row));
        const qint64 fileSize = ShareIndexDb::qi64(result->GetValue(3, row));
        if (tth.isEmpty() || cid.isEmpty() || cids[tth].contains(cid))
            continue;
        if (out[tth].size() >= limitPerTth)
            continue;
        cids[tth].insert(cid);
        out[tth].append(IndexUser{nick.isEmpty() ? cid.left(8) : nick, cid, fileSize});
    }
    return out;
}

#endif
