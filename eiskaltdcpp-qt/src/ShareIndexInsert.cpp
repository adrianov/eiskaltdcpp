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

bool ShareIndex::insertRow(duckdb::Connection &con, const QVariantMap &row, int source)
{
    const bool isDir = row.value("is_dir").toBool();
    const QString tth = sqlText(row.value("tth"));
    const QString cid = sqlText(row.value("cid"));
    const QString path = sqlText(row.value("path"));
    const QString name = sqlText(row.value("name"));
    const QString ext = row.contains("ext") ? sqlText(row.value("ext")) : fileExt(name, isDir);
    const QString stamp = nowStamp();
    const QString ukey = ShareIndexDb::rowUkey(cid, tth, path, name, isDir);

    if (cid.isEmpty() || name.isEmpty())
        return false;

    auto res = con.Query(
        "INSERT OR IGNORE INTO share_entries ("
        "ukey, cid, hub_url, tth, path, name, name_cf, path_cf, ext, is_dir, size, nick, hub_name,"
        "free_slots, all_slots, ip, bitrate, resolution, video_info, audio_info,"
        "shared_ts, source, created_at"
        ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
        ShareIndexDb::strVal(ukey),
        ShareIndexDb::strVal(cid),
        ShareIndexDb::strVal(row.value("hub_url")),
        ShareIndexDb::strVal(tth),
        ShareIndexDb::strVal(path),
        ShareIndexDb::strVal(name),
        ShareIndexDb::strVal(name.toCaseFolded()),
        ShareIndexDb::strVal(path.toCaseFolded()),
        ShareIndexDb::strVal(ext.isEmpty() ? QStringLiteral("") : ext),
        ShareIndexDb::i64Val(isDir ? 1 : 0),
        ShareIndexDb::i64Val(row.value("size").toLongLong()),
        ShareIndexDb::strVal(row.value("nick")),
        ShareIndexDb::strVal(row.value("hub_name")),
        row.contains("free_slots") ? ShareIndexDb::i32Val(row.value("free_slots")) : ShareIndexDb::nullVal(),
        row.contains("all_slots") ? ShareIndexDb::i32Val(row.value("all_slots")) : ShareIndexDb::nullVal(),
        ShareIndexDb::strVal(row.value("ip")),
        row.contains("bitrate") ? ShareIndexDb::i32Val(row.value("bitrate")) : ShareIndexDb::nullVal(),
        ShareIndexDb::strVal(row.value("resolution")),
        ShareIndexDb::strVal(row.value("video_info")),
        ShareIndexDb::strVal(row.value("audio_info")),
        row.contains("shared_ts") ? ShareIndexDb::i64Val(row.value("shared_ts")) : ShareIndexDb::nullVal(),
        ShareIndexDb::i64Val(source),
        ShareIndexDb::strVal(stamp));

    if (res->HasError()) {
        setLastError(QString::fromStdString(res->GetError()));
        return false;
    }
    setLastError(QString());
    return true;
}

bool ShareIndex::appendListRows(duckdb::Connection &con, const QList<QVariantMap> &rows)
{
    try {
        duckdb::Appender app(con, "share_entries");
        app.ClearColumns();
        const char *cols[] = {
            "ukey", "cid", "hub_url", "tth", "path", "name", "name_cf", "path_cf", "ext",
            "is_dir", "size", "nick", "hub_name", "free_slots", "all_slots", "ip",
            "bitrate", "resolution", "video_info", "audio_info", "shared_ts", "source",
            "created_at"
        };
        for (const char *c : cols)
            app.AddColumn(c);

        auto appendStr = [&app](const QString &s) {
            const QByteArray u = s.toUtf8();
            app.Append(u.constData(), uint32_t(u.size()));
        };

        const QString stamp = nowStamp();
        for (const QVariantMap &row : rows) {
            const bool isDir = row.value("is_dir").toBool();
            const QString tth = sqlText(row.value("tth"));
            const QString cid = sqlText(row.value("cid"));
            const QString path = sqlText(row.value("path"));
            const QString name = sqlText(row.value("name"));
            if (cid.isEmpty() || name.isEmpty())
                continue;
            const QString ext = row.contains("ext") ? sqlText(row.value("ext"))
                                                    : fileExt(name, isDir);
            const QString ukey = ShareIndexDb::rowUkey(cid, tth, path, name, isDir);

            app.BeginRow();
            appendStr(ukey);
            appendStr(cid);
            appendStr(sqlText(row.value("hub_url")));
            appendStr(tth);
            appendStr(path);
            appendStr(name);
            appendStr(name.toCaseFolded());
            appendStr(path.toCaseFolded());
            appendStr(ext);
            app.Append(int32_t(isDir ? 1 : 0));
            app.Append(int64_t(row.value("size").toLongLong()));
            appendStr(sqlText(row.value("nick")));
            appendStr(sqlText(row.value("hub_name")));
            if (row.contains("free_slots"))
                app.Append(int32_t(row.value("free_slots").toInt()));
            else
                app.AppendDefault();
            if (row.contains("all_slots"))
                app.Append(int32_t(row.value("all_slots").toInt()));
            else
                app.AppendDefault();
            appendStr(sqlText(row.value("ip")));
            if (row.contains("bitrate"))
                app.Append(int32_t(row.value("bitrate").toInt()));
            else
                app.AppendDefault();
            appendStr(sqlText(row.value("resolution")));
            appendStr(sqlText(row.value("video_info")));
            appendStr(sqlText(row.value("audio_info")));
            if (row.contains("shared_ts"))
                app.Append(int64_t(row.value("shared_ts").toLongLong()));
            else
                app.AppendDefault();
            app.Append(int32_t(SourceFileList));
            appendStr(stamp);
            app.EndRow();
        }
        app.Close();
    } catch (const std::exception &e) {
        setLastError(QString::fromUtf8(e.what()));
        return false;
    }
    setLastError(QString());
    return true;
}

#endif
