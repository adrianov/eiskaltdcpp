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

namespace {

QString sqlText(const QVariant &v)
{
    const QString s = v.toString();
    return s.isNull() ? QStringLiteral("") : s;
}

} // namespace

/** Stage one file-list chunk, normalize shared file/user data, then append
    locations. Equal file strings become NULL (use share_files). */
bool ShareIndex::appendListRows(duckdb::Connection &con, const QList<QVariantMap> &rows)
{
    try {
        if (!ShareIndexDb::execOk(con,
                "CREATE TEMP TABLE IF NOT EXISTS share_stage ("
                "cid TEXT, hub_url TEXT, tth TEXT, path TEXT, name TEXT, "
                "name_cf TEXT, path_cf TEXT, ext TEXT, is_dir INTEGER, size BIGINT, "
                "nick TEXT, hub_name TEXT, ip TEXT, source INTEGER, created_at TEXT,"
                "seq BIGINT)")
                || !ShareIndexDb::execOk(con, "DELETE FROM share_stage"))
            return false;

        duckdb::Appender app(con, "share_stage");
        app.ClearColumns();
        const char *cols[] = {
            "cid", "hub_url", "tth", "path", "name", "name_cf", "path_cf", "ext",
            "is_dir", "size", "nick", "hub_name", "ip", "source", "created_at", "seq"
        };
        for (const char *c : cols)
            app.AddColumn(c);

        auto appendStr = [&app](const QString &s) {
            const QByteArray u = s.toUtf8();
            app.Append(u.constData(), uint32_t(u.size()));
        };

        const QString stamp = nowStamp();
        qint64 seq = 0;
        for (const QVariantMap &row : rows) {
            const bool isDir = row.value("is_dir").toBool();
            const QString cid = sqlText(row.value("cid"));
            const QString path = sqlText(row.value("path"));
            const QString name = sqlText(row.value("name"));
            if (cid.isEmpty() || name.isEmpty())
                continue;
            const QString ext = row.contains("ext") ? sqlText(row.value("ext"))
                                                    : fileExt(name, isDir);

            app.BeginRow();
            appendStr(cid);
            appendStr(sqlText(row.value("hub_url")));
            appendStr(sqlText(row.value("tth")));
            appendStr(path);
            appendStr(name);
            appendStr(name.toCaseFolded());
            appendStr(path.toCaseFolded());
            appendStr(ext);
            app.Append(int32_t(isDir ? 1 : 0));
            app.Append(int64_t(row.value("size").toLongLong()));
            appendStr(sqlText(row.value("nick")));
            appendStr(sqlText(row.value("hub_name")));
            appendStr(sqlText(row.value("ip")));
            app.Append(int32_t(SourceFileList));
            appendStr(stamp);
            app.Append(int64_t(seq++));
            app.EndRow();
        }
        app.Close();

        const char *sql[] = {
            "INSERT INTO share_users "
            "SELECT (SELECT coalesce(max(user_id),0)+1 FROM share_users), cid, hub_url, "
            "nick, hub_name, NULL, NULL, ip, created_at FROM share_stage s "
            "WHERE NOT EXISTS (SELECT 1 FROM share_users u "
            "WHERE u.cid=s.cid AND u.hub_url=s.hub_url) LIMIT 1",
            "UPDATE share_users u SET nick=s.nick, hub_name=s.hub_name, "
            "ip=CASE WHEN s.ip='' THEN u.ip ELSE s.ip END, updated_at=s.created_at "
            "FROM (SELECT * FROM share_stage LIMIT 1) s "
            "WHERE u.cid=s.cid AND u.hub_url=s.hub_url",
            "INSERT INTO share_files "
            "SELECT base + row_number() OVER (), tth, size, name, path, ext, name_cf, path_cf "
            "FROM ("
            "SELECT s.tth, s.size, s.name, s.path, s.ext, s.name_cf, s.path_cf, "
            "row_number() OVER (PARTITION BY s.tth ORDER BY s.seq) AS rn "
            "FROM share_stage s LEFT JOIN share_files f ON f.tth=s.tth "
            "WHERE s.is_dir=0 AND s.tth!='' AND f.file_id IS NULL"
            ") fresh CROSS JOIN (SELECT coalesce(max(file_id), 0) AS base FROM share_files) "
            "WHERE rn = 1",
            "INSERT INTO share_locations "
            "SELECT u.user_id, f.file_id, "
            "CASE WHEN f.file_id IS NULL OR s.path != f.path THEN s.path END, "
            "CASE WHEN f.file_id IS NULL OR s.name != f.name THEN s.name END, "
            "CASE WHEN f.file_id IS NULL OR s.ext != f.ext THEN s.ext END, "
            "s.is_dir, "
            "CASE WHEN f.file_id IS NULL THEN s.size ELSE NULL END, "
            "s.source, s.created_at, "
            "CASE WHEN f.file_id IS NULL OR s.name_cf != f.name_cf THEN s.name_cf END, "
            "CASE WHEN f.file_id IS NULL OR s.path_cf != f.path_cf THEN s.path_cf END "
            "FROM share_stage s LEFT JOIN share_files f "
            "ON f.tth=s.tth "
            "JOIN share_users u ON u.cid=s.cid AND u.hub_url=s.hub_url"
        };
        for (const char *statement : sql) {
            QString err;
            if (!ShareIndexDb::execOk(con, statement, &err)) {
                setLastError(err);
                return false;
            }
        }
    } catch (const std::exception &e) {
        setLastError(QString::fromUtf8(e.what()));
        return false;
    }
    setLastError(QString());
    return true;
}

#endif
