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

bool ShareIndex::upsertRow(QSqlDatabase &db, const QVariantMap &row, int source)
{
    const QString stamp = nowStamp();
    const bool isDir = row.value("is_dir").toBool();
    const QString tth = row.value("tth").toString();
    const QString cid = row.value("cid").toString();
    const QString path = row.value("path").toString();
    const QString name = row.value("name").toString();
    const QString ext = row.contains("ext")
            ? row.value("ext").toString()
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
        lastSqlError = find.lastError().text();
        return false;
    }

    if (find.next()) {
        const qint64 id = find.value(0).toLongLong();
        const int oldSource = find.value(1).toInt();
        // Never demote a file-list row to hub-search (list refresh deletes by source=1).
        const int keepSource = (oldSource == SourceFileList || source == SourceFileList)
                ? SourceFileList : source;
        const bool fromList = (source == SourceFileList);

        QSqlQuery upd(db);
        if (fromList) {
            upd.prepare(
                "UPDATE share_entries SET hub_url=?, tth=?, path=?, name=?, ext=?, is_dir=?, size=?,"
                "nick=?, hub_name=?, free_slots=?, all_slots=?, ip=?, bitrate=?, resolution=?,"
                "video_info=?, audio_info=?, hit=?, shared_ts=?, source=?, updated_at=? WHERE id=?");
        } else {
            // Hub hit: refresh display fields; keep media from a prior file list.
            upd.prepare(
                "UPDATE share_entries SET hub_url=?, tth=?, path=?, name=?, ext=?, is_dir=?, size=?,"
                "nick=?, hub_name=?,"
                "free_slots=COALESCE(?, free_slots), all_slots=COALESCE(?, all_slots),"
                "ip=CASE WHEN ? = '' THEN ip ELSE ? END,"
                "source=?, updated_at=? WHERE id=?");
        }

        upd.addBindValue(row.value("hub_url").toString());
        upd.addBindValue(tth);
        upd.addBindValue(path);
        upd.addBindValue(name);
        upd.addBindValue(ext.isEmpty() ? QStringLiteral("") : ext);
        upd.addBindValue(isDir ? 1 : 0);
        upd.addBindValue(row.value("size").toLongLong());
        upd.addBindValue(row.value("nick").toString());
        upd.addBindValue(row.value("hub_name").toString());

        if (fromList) {
            upd.addBindValue(row.contains("free_slots") ? row.value("free_slots") : QVariant());
            upd.addBindValue(row.contains("all_slots") ? row.value("all_slots") : QVariant());
            upd.addBindValue(row.value("ip").toString());
            upd.addBindValue(row.contains("bitrate") ? row.value("bitrate") : QVariant());
            upd.addBindValue(row.value("resolution").toString());
            upd.addBindValue(row.value("video_info").toString());
            upd.addBindValue(row.value("audio_info").toString());
            upd.addBindValue(row.contains("hit") ? row.value("hit") : QVariant());
            upd.addBindValue(row.contains("shared_ts") ? row.value("shared_ts") : QVariant());
            upd.addBindValue(keepSource);
            upd.addBindValue(stamp);
            upd.addBindValue(id);
        } else {
            upd.addBindValue(row.contains("free_slots") ? row.value("free_slots") : QVariant());
            upd.addBindValue(row.contains("all_slots") ? row.value("all_slots") : QVariant());
            const QString ip = row.value("ip").toString();
            upd.addBindValue(ip);
            upd.addBindValue(ip);
            upd.addBindValue(keepSource);
            upd.addBindValue(stamp);
            upd.addBindValue(id);
        }

        if (!upd.exec()) {
            lastSqlError = upd.lastError().text();
            return false;
        }
        lastSqlError.clear();
        return true;
    }

    QSqlQuery ins(db);
    ins.prepare(
        "INSERT INTO share_entries ("
        "cid, hub_url, tth, path, name, ext, is_dir, size, nick, hub_name,"
        "free_slots, all_slots, ip, bitrate, resolution, video_info, audio_info,"
        "hit, shared_ts, source, created_at, updated_at"
        ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
    ins.addBindValue(cid);
    ins.addBindValue(row.value("hub_url").toString());
    ins.addBindValue(tth);
    ins.addBindValue(path);
    ins.addBindValue(name);
    ins.addBindValue(ext.isEmpty() ? QStringLiteral("") : ext);
    ins.addBindValue(isDir ? 1 : 0);
    ins.addBindValue(row.value("size").toLongLong());
    ins.addBindValue(row.value("nick").toString());
    ins.addBindValue(row.value("hub_name").toString());
    ins.addBindValue(row.contains("free_slots") ? row.value("free_slots") : QVariant());
    ins.addBindValue(row.contains("all_slots") ? row.value("all_slots") : QVariant());
    ins.addBindValue(row.value("ip").toString());
    ins.addBindValue(row.contains("bitrate") ? row.value("bitrate") : QVariant());
    ins.addBindValue(row.value("resolution").toString());
    ins.addBindValue(row.value("video_info").toString());
    ins.addBindValue(row.value("audio_info").toString());
    ins.addBindValue(row.contains("hit") ? row.value("hit") : QVariant());
    ins.addBindValue(row.contains("shared_ts") ? row.value("shared_ts") : QVariant());
    ins.addBindValue(source);
    ins.addBindValue(stamp);
    ins.addBindValue(stamp);
    if (!ins.exec()) {
        lastSqlError = ins.lastError().text();
        return false;
    }
    lastSqlError.clear();
    return true;
}

void ShareIndex::upsertFromSearch(const QVariantMap &map)
{
    open();
    if (!opened)
        return;

    const bool isDir = map.value("ISDIR").toBool();
    const QString name = map.value("FILE").toString();

    QVariantMap row;
    row["cid"] = map.value("CID");
    row["hub_url"] = map.value("HOST");
    row["tth"] = map.value("TTH");
    row["path"] = map.value("PATH");
    row["name"] = name;
    row["ext"] = fileExt(name, isDir);
    row["is_dir"] = isDir;
    row["size"] = map.value("SIZE");
    row["nick"] = map.value("NICK");
    row["hub_name"] = map.value("HUB");
    row["free_slots"] = map.value("FSLS");
    row["all_slots"] = map.value("ASLS");
    row["ip"] = map.value("IP");

    QMutexLocker lock(&mutex);
    QSqlDatabase db = QSqlDatabase::database("ShareIndex");
    if (!db.isOpen())
        return;
    upsertRow(db, row, SourceHubSearch);
}

#endif
