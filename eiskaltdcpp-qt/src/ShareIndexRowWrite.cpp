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

/** Normalize one result, then UPDATE-else-INSERT its scalar location.
    Keeps created_at and file-list source; empty ip / NULL slots keep old. */
bool ShareIndex::upsertRow(duckdb::Connection &con, const QVariantMap &row, int source)
{
    const bool isDir = row.value("is_dir").toBool();
    const QString tth = sqlText(row.value("tth"));
    const QString cid = sqlText(row.value("cid"));
    const QString path = sqlText(row.value("path"));
    const QString name = sqlText(row.value("name"));
    const QString ext = row.contains("ext") ? sqlText(row.value("ext")) : fileExt(name, isDir);
    const QString ip = sqlText(row.value("ip"));
    if (cid.isEmpty() || name.isEmpty())
        return false;

    const bool byTth = !isDir && !tth.isEmpty();
    const qint64 size = row.value("size").toLongLong();
    const QString hubUrl = sqlText(row.value("hub_url"));
    const QString stamp = nowStamp();
    QString err;
    duckdb::vector<duckdb::Value> user;
    user.push_back(ShareIndexDb::strVal(cid));
    user.push_back(ShareIndexDb::strVal(hubUrl));
    user.push_back(ShareIndexDb::strVal(row.value("nick")));
    user.push_back(ShareIndexDb::strVal(row.value("hub_name")));
    user.push_back(row.contains("free_slots") ? ShareIndexDb::i32Val(row.value("free_slots"))
                                              : ShareIndexDb::nullVal());
    user.push_back(row.contains("all_slots") ? ShareIndexDb::i32Val(row.value("all_slots"))
                                             : ShareIndexDb::nullVal());
    user.push_back(ShareIndexDb::strVal(ip));
    user.push_back(ShareIndexDb::strVal(stamp));
    user.push_back(ShareIndexDb::strVal(cid));
    user.push_back(ShareIndexDb::strVal(hubUrl));
    if (!ShareIndexDb::queryMat(con,
            "INSERT INTO share_users "
            "SELECT (SELECT coalesce(max(user_id),0)+1 FROM share_users), ?,?,?,?,?,?,?,? "
            "WHERE NOT EXISTS (SELECT 1 FROM share_users WHERE cid=? AND hub_url=?)",
            user, &err)) {
        setLastError(err);
        return false;
    }
    duckdb::vector<duckdb::Value> updateUser;
    for (int i = 2; i <= 6; ++i)
        updateUser.push_back(user[i]);
    updateUser.push_back(user[6]);
    updateUser.push_back(user[7]);
    updateUser.push_back(ShareIndexDb::strVal(cid));
    updateUser.push_back(ShareIndexDb::strVal(hubUrl));
    if (!ShareIndexDb::queryMat(con,
            "UPDATE share_users SET nick=?, hub_name=?, "
            "free_slots=COALESCE(?,free_slots), all_slots=COALESCE(?,all_slots), "
            "ip=CASE WHEN ?='' THEN ip ELSE ? END, updated_at=? WHERE cid=? AND hub_url=?",
            updateUser, &err)) {
        setLastError(err);
        return false;
    }
    auto userRow = ShareIndexDb::query2(con,
        "SELECT user_id FROM share_users WHERE cid=? AND hub_url=?",
        ShareIndexDb::strVal(cid), ShareIndexDb::strVal(hubUrl), &err);
    if (!userRow || userRow->RowCount() == 0) {
        setLastError(err.isEmpty() ? QStringLiteral("user id") : err);
        return false;
    }
    const qint64 userId = ShareIndexDb::qi64(userRow->GetValue(0, 0));

    qint64 fileId = 0;
    if (byTth) {
        duckdb::vector<duckdb::Value> addFile;
        addFile.push_back(ShareIndexDb::strVal(tth));
        addFile.push_back(ShareIndexDb::i64Val(size));
        addFile.push_back(ShareIndexDb::strVal(tth));
        addFile.push_back(ShareIndexDb::i64Val(size));
        if (!ShareIndexDb::queryMat(con,
                "INSERT INTO share_files "
                "SELECT (SELECT coalesce(max(file_id),0)+1 FROM share_files), ?, ? "
                "WHERE NOT EXISTS (SELECT 1 FROM share_files WHERE tth=? AND size=?)",
                addFile, &err)) {
            setLastError(err);
            return false;
        }
        auto found = ShareIndexDb::query2(con,
            "SELECT file_id FROM share_files WHERE tth=? AND size=?",
            ShareIndexDb::strVal(tth), ShareIndexDb::i64Val(size), &err);
        if (!found || found->RowCount() == 0) {
            setLastError(err.isEmpty() ? QStringLiteral("file id") : err);
            return false;
        }
        fileId = ShareIndexDb::qi64(found->GetValue(0, 0));
    }

    duckdb::vector<duckdb::Value> binds;
    binds.push_back(ShareIndexDb::i64Val(userId));
    binds.push_back(byTth ? ShareIndexDb::i64Val(fileId) : ShareIndexDb::nullVal());
    binds.push_back(ShareIndexDb::strVal(path));
    binds.push_back(ShareIndexDb::strVal(name));
    binds.push_back(ShareIndexDb::strVal(name.toCaseFolded()));
    binds.push_back(ShareIndexDb::strVal(path.toCaseFolded()));
    binds.push_back(ShareIndexDb::strVal(ext));
    binds.push_back(byTth ? ShareIndexDb::nullVal() : ShareIndexDb::i64Val(size));
    binds.push_back(ShareIndexDb::i64Val(source));
    binds.push_back(ShareIndexDb::i64Val(source));
    binds.push_back(ShareIndexDb::i64Val(userId));
    std::string sql =
        "UPDATE share_locations SET user_id=?, file_id=?, path=?, name=?, name_cf=?, path_cf=?, ext=?,"
        " local_size=?, source=CASE WHEN source=1 OR ?=1 THEN 1 ELSE ? END "
        "WHERE user_id=?";
    if (byTth) {
        sql += " AND file_id IN (SELECT file_id FROM share_files WHERE tth=?)";
        binds.push_back(ShareIndexDb::strVal(tth));
    } else {
        sql += " AND file_id IS NULL AND path=? AND name=? AND is_dir=?";
        binds.push_back(ShareIndexDb::strVal(path));
        binds.push_back(ShareIndexDb::strVal(name));
        binds.push_back(ShareIndexDb::i64Val(isDir ? 1 : 0));
    }

    auto upd = ShareIndexDb::queryMat(con, sql, binds, &err);
    if (!upd) {
        setLastError(err);
        return false;
    }
    if (upd->RowCount() > 0 && ShareIndexDb::qi64(upd->GetValue(0, 0)) > 0) {
        setLastError(QString());
        return true;
    }

    duckdb::vector<duckdb::Value> ins;
    ins.push_back(ShareIndexDb::i64Val(userId));
    ins.push_back(byTth ? ShareIndexDb::i64Val(fileId) : ShareIndexDb::nullVal());
    ins.push_back(ShareIndexDb::strVal(path));
    ins.push_back(ShareIndexDb::strVal(name));
    ins.push_back(ShareIndexDb::strVal(ext));
    ins.push_back(ShareIndexDb::i64Val(isDir ? 1 : 0));
    ins.push_back(byTth ? ShareIndexDb::nullVal() : ShareIndexDb::i64Val(size));
    ins.push_back(ShareIndexDb::i64Val(source));
    ins.push_back(ShareIndexDb::strVal(stamp));
    ins.push_back(ShareIndexDb::strVal(name.toCaseFolded()));
    ins.push_back(ShareIndexDb::strVal(path.toCaseFolded()));
    auto res = ShareIndexDb::queryMat(con,
        "INSERT INTO share_locations ("
        "user_id, file_id, path, name, ext, is_dir, local_size, source, created_at,"
        " name_cf, path_cf) VALUES (?,?,?,?,?,?,?,?,?,?,?)", ins, &err);
    if (!res) {
        setLastError(err);
        return false;
    }
    setLastError(QString());
    return true;
}

#endif
