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

/** Upsert share_users and return user_id, or -1 on error. */
qint64 ShareIndex::ensureUserId(duckdb::Connection &con, const QVariantMap &row,
                                const QString &stamp)
{
    const QString cid = sqlText(row.value("cid"));
    const QString hubUrl = sqlText(row.value("hub_url"));
    const QString ip = sqlText(row.value("ip"));
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
        return -1;
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
        return -1;
    }
    auto userRow = ShareIndexDb::query2(con,
        "SELECT user_id FROM share_users WHERE cid=? AND hub_url=?",
        ShareIndexDb::strVal(cid), ShareIndexDb::strVal(hubUrl), &err);
    if (!userRow || userRow->RowCount() == 0) {
        setLastError(err.isEmpty() ? QStringLiteral("user id") : err);
        return -1;
    }
    return ShareIndexDb::qi64(userRow->GetValue(0, 0));
}

/** Insert first-found share_files row; fill canon strings. Returns file_id or -1. */
qint64 ShareIndex::ensureFileId(duckdb::Connection &con, const QString &tth, qint64 size,
                                const QString &name, const QString &path, const QString &ext,
                                QString *cName, QString *cPath, QString *cExt,
                                QString *cNameCf, QString *cPathCf)
{
    const QString nameCf = name.toCaseFolded();
    const QString pathCf = path.toCaseFolded();
    QString err;
    duckdb::vector<duckdb::Value> addFile;
    addFile.push_back(ShareIndexDb::strVal(tth));
    addFile.push_back(ShareIndexDb::i64Val(size));
    addFile.push_back(ShareIndexDb::strVal(name));
    addFile.push_back(ShareIndexDb::strVal(path));
    addFile.push_back(ShareIndexDb::strVal(ext));
    addFile.push_back(ShareIndexDb::strVal(nameCf));
    addFile.push_back(ShareIndexDb::strVal(pathCf));
    addFile.push_back(ShareIndexDb::strVal(tth));
    if (!ShareIndexDb::queryMat(con,
            "INSERT INTO share_files "
            "SELECT (SELECT coalesce(max(file_id),0)+1 FROM share_files), "
            "?,?,?,?,?,?,? WHERE NOT EXISTS ("
            "SELECT 1 FROM share_files WHERE tth=?)",
            addFile, &err)) {
        setLastError(err);
        return -1;
    }
    auto found = ShareIndexDb::query1(con,
        "SELECT file_id, name, path, ext, name_cf, path_cf "
        "FROM share_files WHERE tth=? ORDER BY file_id LIMIT 1",
        ShareIndexDb::strVal(tth), &err);
    if (!found || found->RowCount() == 0) {
        setLastError(err.isEmpty() ? QStringLiteral("file id") : err);
        return -1;
    }
    *cName = ShareIndexDb::qstr(found->GetValue(1, 0));
    *cPath = ShareIndexDb::qstr(found->GetValue(2, 0));
    *cExt = ShareIndexDb::qstr(found->GetValue(3, 0));
    *cNameCf = ShareIndexDb::qstr(found->GetValue(4, 0));
    *cPathCf = ShareIndexDb::qstr(found->GetValue(5, 0));
    return ShareIndexDb::qi64(found->GetValue(0, 0));
}

#endif
