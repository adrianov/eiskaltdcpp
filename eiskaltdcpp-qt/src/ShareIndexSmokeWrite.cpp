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

#include <QElapsedTimer>

/** Canonical first-found strings, NULL overrides, and warm search timing. */
bool shareIndexSmokeWrites(ShareIndex &idx, duckdb::Connection &con, QString *error)
{
    QVariantMap file;
    file["cid"] = "CIDTEST";
    file["hub_url"] = "adc://hub";
    file["tth"] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    file["path"] = "TV\\Breaking Bad\\";
    file["name"] = "S01E01.mkv";
    file["is_dir"] = false;
    file["size"] = 12345;
    file["nick"] = "Alice";
    file["hub_name"] = "TestHub";
    if (!idx.upsertRow(con, file, ShareIndex::SourceFileList)) {
        if (error)
            *error = QString("insert file: %1").arg(idx.lastError());
        return false;
    }

    auto extq = con.Query(
        "SELECT coalesce(l.ext,f.ext), l.name, l.path, f.name "
        "FROM share_locations l JOIN share_files f USING(file_id) WHERE f.tth != ''");
    if (extq->HasError() || extq->RowCount() == 0
            || ShareIndexDb::qstr(extq->GetValue(0, 0)) != QLatin1String("MKV")
            || !extq->GetValue(1, 0).IsNull() || !extq->GetValue(2, 0).IsNull()
            || ShareIndexDb::qstr(extq->GetValue(3, 0)) != QLatin1String("S01E01.mkv")) {
        if (error)
            *error = "canonical / NULL override mismatch";
        return false;
    }

    QVariantMap dirProp;
    dirProp["cid"] = "CIDTEST";
    dirProp["hub_url"] = "adc://hub";
    dirProp["tth"] = "";
    dirProp["path"] = "TV\\";
    dirProp["name"] = "Breaking Bad";
    dirProp["is_dir"] = true;
    dirProp["size"] = 0;
    dirProp["nick"] = "Alice";
    dirProp["hub_name"] = "TestHub";
    if (!idx.upsertRow(con, dirProp, ShareIndex::SourceFileList)) {
        if (error)
            *error = QString("insert dir: %1").arg(idx.lastError());
        return false;
    }

    con.Query(
        "UPDATE share_locations SET created_at='2000-01-01 00:00:00' "
        "WHERE file_id IS NOT NULL");

    file["name"] = "S01E01_renamed.mkv";
    file["size"] = 99999; // Same TTH must still reuse the first file row.
    if (!idx.upsertRow(con, file, ShareIndex::SourceHubSearch)) {
        if (error)
            *error = "upsert";
        return false;
    }

    auto created = con.Query(
        "SELECT l.created_at, coalesce(l.name,f.name), l.name, f.name, f.size, l.source, "
        "(SELECT count(*) FROM share_files WHERE tth='ABCDEFGHIJKLMNOPQRSTUVWXYZ234567') "
        "FROM share_locations l JOIN share_files f USING(file_id) "
        "WHERE f.tth='ABCDEFGHIJKLMNOPQRSTUVWXYZ234567'");
    if (created->HasError() || created->RowCount() == 0
            || ShareIndexDb::qstr(created->GetValue(0, 0)) != QLatin1String("2000-01-01 00:00:00")
            || ShareIndexDb::qstr(created->GetValue(1, 0)) != QLatin1String("S01E01_renamed.mkv")
            || created->GetValue(2, 0).IsNull()
            || ShareIndexDb::qstr(created->GetValue(3, 0)) != QLatin1String("S01E01.mkv")
            || ShareIndexDb::qi64(created->GetValue(4, 0)) != 12345
            || ShareIndexDb::qi64(created->GetValue(5, 0)) != ShareIndex::SourceFileList
            || ShareIndexDb::qi64(created->GetValue(6, 0)) != 1) {
        if (error)
            *error = "upsert created_at/name/source/single-tth";
        return false;
    }

    QVariantMap other;
    other["cid"] = "CIDOTHER";
    other["hub_url"] = "adc://hub2";
    other["tth"] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    other["path"] = "Downloads\\";
    other["name"] = "S01E01.mkv";
    other["is_dir"] = false;
    other["size"] = 99999;
    other["nick"] = "Bob";
    other["hub_name"] = "Hub2";
    if (!idx.upsertRow(con, other, ShareIndex::SourceFileList)) {
        if (error)
            *error = QString("insert override: %1").arg(idx.lastError());
        return false;
    }
    auto ov = con.Query(
        "SELECT l.name, l.path FROM share_locations l "
        "JOIN share_users u USING(user_id) WHERE u.cid='CIDOTHER'");
    if (ov->HasError() || ov->RowCount() != 1 || !ov->GetValue(0, 0).IsNull()
            || ShareIndexDb::qstr(ov->GetValue(1, 0)) != QLatin1String("Downloads\\")) {
        if (error)
            *error = "path override / name NULL expected";
        return false;
    }

    ShareIndex::SearchFilter f;
    f.terms << "breaking";
    for (int i = 0; i < 3; ++i)
        idx.searchFts(con, f);
    QElapsedTimer timer;
    timer.start();
    const auto hits = idx.searchFts(con, f);
    const qint64 ms = timer.elapsed();
    if (hits.isEmpty()) {
        if (error)
            *error = "timing search empty";
        return false;
    }
    auto plan = con.Query(
        "EXPLAIN SELECT coalesce(e.name,f.name) FROM share_locations e "
        "JOIN share_users u ON u.user_id=e.user_id "
        "LEFT JOIN share_files f ON f.file_id=e.file_id "
        "WHERE contains(coalesce(e.name_cf,f.name_cf), 'breaking') LIMIT 10");
    if (plan->HasError()) {
        if (error)
            *error = QString::fromStdString(plan->GetError());
        return false;
    }
    if (ms > 2000) {
        if (error)
            *error = QStringLiteral("search too slow: %1 ms").arg(ms);
        return false;
    }
    return true;
}

#endif
