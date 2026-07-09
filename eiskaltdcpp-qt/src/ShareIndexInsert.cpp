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

bool ShareIndex::prepareInsert(QSqlQuery &ins) const
{
    return ins.prepare(
        "INSERT OR IGNORE INTO share_entries ("
        "cid, hub_url, tth, path, name, name_cf, path_cf, ext, is_dir, size, nick, hub_name,"
        "free_slots, all_slots, ip, bitrate, resolution, video_info, audio_info,"
        "hit, shared_ts, source, created_at, updated_at"
        ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
}

bool ShareIndex::bindInsert(QSqlQuery &ins, const QVariantMap &row, int source) const
{
    const bool isDir = row.value("is_dir").toBool();
    const QString tth = sqlText(row.value("tth"));
    const QString cid = sqlText(row.value("cid"));
    const QString path = sqlText(row.value("path"));
    const QString name = sqlText(row.value("name"));
    const QString ext = row.contains("ext")
            ? sqlText(row.value("ext"))
            : fileExt(name, isDir);
    const QString stamp = nowStamp();

    if (cid.isEmpty() || name.isEmpty())
        return false;

    // Positional binds so a reused prepared statement does not accumulate values.
    int i = 0;
    ins.bindValue(i++, cid);
    ins.bindValue(i++, sqlText(row.value("hub_url")));
    ins.bindValue(i++, tth);
    ins.bindValue(i++, path);
    ins.bindValue(i++, name);
    ins.bindValue(i++, name.toCaseFolded());
    ins.bindValue(i++, path.toCaseFolded());
    ins.bindValue(i++, ext.isEmpty() ? QStringLiteral("") : ext);
    ins.bindValue(i++, isDir ? 1 : 0);
    ins.bindValue(i++, row.value("size").toLongLong());
    ins.bindValue(i++, sqlText(row.value("nick")));
    ins.bindValue(i++, sqlText(row.value("hub_name")));
    ins.bindValue(i++, row.contains("free_slots") ? row.value("free_slots") : QVariant());
    ins.bindValue(i++, row.contains("all_slots") ? row.value("all_slots") : QVariant());
    ins.bindValue(i++, sqlText(row.value("ip")));
    ins.bindValue(i++, row.contains("bitrate") ? row.value("bitrate") : QVariant());
    ins.bindValue(i++, sqlText(row.value("resolution")));
    ins.bindValue(i++, sqlText(row.value("video_info")));
    ins.bindValue(i++, sqlText(row.value("audio_info")));
    ins.bindValue(i++, row.contains("hit") ? row.value("hit") : QVariant());
    ins.bindValue(i++, row.contains("shared_ts") ? row.value("shared_ts") : QVariant());
    ins.bindValue(i++, source);
    ins.bindValue(i++, stamp);
    ins.bindValue(i++, stamp);
    return true;
}

bool ShareIndex::insertRow(QSqlDatabase &db, const QVariantMap &row, int source)
{
    QSqlQuery ins(db);
    if (!prepareInsert(ins) || !bindInsert(ins, row, source))
        return false;
    if (!ins.exec()) {
        setLastError(ins.lastError().text());
        return false;
    }
    setLastError(QString());
    return true;
}

#endif
