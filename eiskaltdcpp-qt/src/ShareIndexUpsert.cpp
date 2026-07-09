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

bool ShareIndex::upsertRow(QSqlDatabase &db, const QVariantMap &row, int source)
{
    const QString stamp = nowStamp();
    const bool isDir = row.value("is_dir").toBool();
    const QString tth = sqlText(row.value("tth"));
    const QString cid = sqlText(row.value("cid"));
    const QString path = sqlText(row.value("path"));
    const QString name = sqlText(row.value("name"));
    const QString ext = row.contains("ext")
            ? sqlText(row.value("ext"))
            : fileExt(name, isDir);

    if (cid.isEmpty() || name.isEmpty())
        return false;

    QSqlQuery find(db);
    if (!isDir && !tth.isEmpty()) {
        find.prepare("SELECT id, source FROM share_entries WHERE cid = ? AND tth = ? AND is_dir = 0");
        find.addBindValue(cid);
        find.addBindValue(tth);
    } else {
        find.prepare("SELECT id, source FROM share_entries WHERE cid = ? AND path = ? AND name = ? AND is_dir = ?");
        find.addBindValue(cid);
        find.addBindValue(path);
        find.addBindValue(name);
        find.addBindValue(isDir ? 1 : 0);
    }

    if (!find.exec()) {
        setLastError(find.lastError().text());
        return false;
    }

    if (!find.next())
        return insertRow(db, row, source);

    const qint64 id = find.value(0).toLongLong();
    const int oldSource = find.value(1).toInt();
    const int keepSource = (oldSource == SourceFileList || source == SourceFileList)
            ? SourceFileList : source;
    const bool fromList = (source == SourceFileList);

    QSqlQuery upd(db);
    if (fromList) {
        upd.prepare(
            "UPDATE share_entries SET hub_url=?, tth=?, path=?, name=?, name_cf=?, path_cf=?, ext=?,"
            "is_dir=?, size=?, nick=?, hub_name=?, free_slots=?, all_slots=?, ip=?, bitrate=?,"
            "resolution=?, video_info=?, audio_info=?, hit=?, shared_ts=?, source=?, updated_at=? WHERE id=?");
    } else {
        upd.prepare(
            "UPDATE share_entries SET hub_url=?, tth=?, path=?, name=?, name_cf=?, path_cf=?, ext=?,"
            "is_dir=?, size=?, nick=?, hub_name=?,"
            "free_slots=COALESCE(?, free_slots), all_slots=COALESCE(?, all_slots),"
            "ip=CASE WHEN ? = '' THEN ip ELSE ? END,"
            "source=?, updated_at=? WHERE id=?");
    }

    upd.addBindValue(sqlText(row.value("hub_url")));
    upd.addBindValue(tth);
    upd.addBindValue(path);
    upd.addBindValue(name);
    upd.addBindValue(name.toCaseFolded());
    upd.addBindValue(path.toCaseFolded());
    upd.addBindValue(ext.isEmpty() ? QStringLiteral("") : ext);
    upd.addBindValue(isDir ? 1 : 0);
    upd.addBindValue(row.value("size").toLongLong());
    upd.addBindValue(sqlText(row.value("nick")));
    upd.addBindValue(sqlText(row.value("hub_name")));

    if (fromList) {
        upd.addBindValue(row.contains("free_slots") ? row.value("free_slots") : QVariant());
        upd.addBindValue(row.contains("all_slots") ? row.value("all_slots") : QVariant());
        upd.addBindValue(sqlText(row.value("ip")));
        upd.addBindValue(row.contains("bitrate") ? row.value("bitrate") : QVariant());
        upd.addBindValue(sqlText(row.value("resolution")));
        upd.addBindValue(sqlText(row.value("video_info")));
        upd.addBindValue(sqlText(row.value("audio_info")));
        upd.addBindValue(row.contains("hit") ? row.value("hit") : QVariant());
        upd.addBindValue(row.contains("shared_ts") ? row.value("shared_ts") : QVariant());
        upd.addBindValue(keepSource);
        upd.addBindValue(stamp);
        upd.addBindValue(id);
    } else {
        upd.addBindValue(row.contains("free_slots") ? row.value("free_slots") : QVariant());
        upd.addBindValue(row.contains("all_slots") ? row.value("all_slots") : QVariant());
        const QString ip = sqlText(row.value("ip"));
        upd.addBindValue(ip);
        upd.addBindValue(ip);
        upd.addBindValue(keepSource);
        upd.addBindValue(stamp);
        upd.addBindValue(id);
    }

    if (!upd.exec()) {
        setLastError(upd.lastError().text());
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

    QSqlDatabase db = threadDb();
    if (!db.isOpen())
        return;

    db.transaction();
    for (const QVariantMap &map : maps) {
        if (ShareIndexWriteQueue::isStopping()) {
            db.rollback();
            return;
        }
        if (!upsertRow(db, searchMapToRow(map), SourceHubSearch)) {
            db.rollback();
            return;
        }
    }
    if (!db.commit())
        setLastError(db.lastError().text());
}

#endif
