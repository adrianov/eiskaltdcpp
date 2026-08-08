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

QHash<QString, ShareIndex::MediaInfo>
ShareIndex::mediaByTth(const QStringList &tths)
{
    QHash<QString, MediaInfo> out;
    if (tths.isEmpty() || !isOpen())
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
        "SELECT tth, coalesce(bitrate,0), coalesce(resolution,''), "
        "coalesce(video,''), coalesce(audio,'') FROM share_files "
        "WHERE tth IN (" + bindMarks(n) + ") AND "
        "(coalesce(bitrate,0)>0 OR coalesce(resolution,'')!='' "
        "OR coalesce(video,'')!='' OR coalesce(audio,'')!='')";
    duckdb::vector<duckdb::Value> binds;
    for (const QString &t : unique)
        binds.push_back(ShareIndexDb::strVal(t));

    QString error;
    auto result = ShareIndexDb::queryMat(*con, sql, binds, &error);
    if (!result) {
        setLastError(error);
        return out;
    }
    setLastError(QString());

    for (idx_t row = 0; row < result->RowCount(); ++row) {
        const QString tth = ShareIndexDb::qstr(result->GetValue(0, row));
        if (tth.isEmpty())
            continue;
        MediaInfo m;
        m.bitrate = int(ShareIndexDb::qi64(result->GetValue(1, row)));
        m.resolution = ShareIndexDb::qstr(result->GetValue(2, row));
        m.video = ShareIndexDb::qstr(result->GetValue(3, row));
        m.audio = ShareIndexDb::qstr(result->GetValue(4, row));
        if (!m.isEmpty())
            out.insert(tth, m);
    }
    return out;
}

#endif
