/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ShareIndex.h"
#include "ShareIndexQueueCore.h"

#ifdef USE_QT_SQLITE

namespace {

QString sqlText(const QVariant &v)
{
    const QString s = v.toString();
    return s.isNull() ? QStringLiteral("") : s;
}

} // namespace

bool ShareIndex::upsertRow(duckdb::Connection &con, const QVariantMap &row, int source)
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

    const bool fromList = (source == SourceFileList);

    // created_at only on insert; ON CONFLICT keeps existing created_at.
    std::string sql;
    if (fromList) {
        sql = "INSERT INTO share_entries ("
              "ukey, cid, hub_url, tth, path, name, name_cf, path_cf, ext, is_dir, size, nick, hub_name,"
              "free_slots, all_slots, ip, bitrate, resolution, video_info, audio_info,"
              "shared_ts, source, created_at"
              ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?) "
              "ON CONFLICT(ukey) DO UPDATE SET "
              "hub_url=excluded.hub_url, tth=excluded.tth, path=excluded.path, name=excluded.name,"
              "name_cf=excluded.name_cf, path_cf=excluded.path_cf, ext=excluded.ext,"
              "is_dir=excluded.is_dir, size=excluded.size, nick=excluded.nick, hub_name=excluded.hub_name,"
              "free_slots=excluded.free_slots, all_slots=excluded.all_slots, ip=excluded.ip,"
              "bitrate=excluded.bitrate, resolution=excluded.resolution, video_info=excluded.video_info,"
              "audio_info=excluded.audio_info, shared_ts=excluded.shared_ts,"
              "source=CASE WHEN share_entries.source=1 OR excluded.source=1 THEN 1 ELSE excluded.source END";
    } else {
        sql = "INSERT INTO share_entries ("
              "ukey, cid, hub_url, tth, path, name, name_cf, path_cf, ext, is_dir, size, nick, hub_name,"
              "free_slots, all_slots, ip, bitrate, resolution, video_info, audio_info,"
              "shared_ts, source, created_at"
              ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?) "
              "ON CONFLICT(ukey) DO UPDATE SET "
              "hub_url=excluded.hub_url, tth=excluded.tth, path=excluded.path, name=excluded.name,"
              "name_cf=excluded.name_cf, path_cf=excluded.path_cf, ext=excluded.ext,"
              "is_dir=excluded.is_dir, size=excluded.size, nick=excluded.nick, hub_name=excluded.hub_name,"
              "free_slots=COALESCE(excluded.free_slots, share_entries.free_slots),"
              "all_slots=COALESCE(excluded.all_slots, share_entries.all_slots),"
              "ip=CASE WHEN excluded.ip = '' THEN share_entries.ip ELSE excluded.ip END,"
              "source=CASE WHEN share_entries.source=1 OR excluded.source=1 THEN 1 ELSE excluded.source END";
    }

    auto res = con.Query(
        sql,
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

namespace {

QVariantMap searchMapToRow(const QVariantMap &map)
{
    const bool isDir = map.value("ISDIR").toBool();
    const QString name = map.value("FILE").toString();

    QVariantMap row;
    row["cid"] = map.value("CID");
    row["hub_url"] = map.value("HOST");
    row["tth"] = map.value("TTH");
    row["path"] = map.value("PATH");
    row["name"] = name;
    row["ext"] = ShareIndex::fileExt(name, isDir);
    row["is_dir"] = isDir;
    row["size"] = map.value("SIZE");
    row["nick"] = map.value("NICK");
    row["hub_name"] = map.value("HUB");
    row["free_slots"] = map.value("FSLS");
    row["all_slots"] = map.value("ASLS");
    row["ip"] = map.value("IP");
    return row;
}

} // namespace

void ShareIndex::upsertFromSearchSync(const QVariantMap &map)
{
    upsertFromSearchBatchSync(QList<QVariantMap>() << map);
}

void ShareIndex::upsertFromSearchBatchSync(const QList<QVariantMap> &maps)
{
    if (maps.isEmpty())
        return;

    open();
    if (!isOpen())
        return;

    duckdb::Connection *con = threadConn();
    if (!con)
        return;

    if (!ShareIndexDb::execOk(*con, "BEGIN TRANSACTION"))
        return;
    for (const QVariantMap &map : maps) {
        if (ShareIndexWriteQueue::isStopping()) {
            ShareIndexDb::execOk(*con, "ROLLBACK");
            return;
        }
        if (!upsertRow(*con, searchMapToRow(map), SourceHubSearch)) {
            ShareIndexDb::execOk(*con, "ROLLBACK");
            return;
        }
    }
    if (!ShareIndexDb::execOk(*con, "COMMIT")) {
        setLastError(QStringLiteral("commit failed"));
        return;
    }
    refreshEntryCount(*con);
    // No CHECKPOINT here: hub SRs are frequent; vacuum after list ingest only.
}

#endif
