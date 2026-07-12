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

bool fail(QString *error, const QString &message)
{
    if (error)
        *error = message;
    return false;
}

bool checkEmptyRebuild(ShareIndex &idx, duckdb::Connection &con, QString *error)
{
    if (!idx.isOpen())
        return fail(error, QStringLiteral("migration open: %1").arg(idx.lastError()));

    auto legacy = con.Query(
        "SELECT 1 FROM information_schema.tables "
        "WHERE table_schema='main' AND table_name='share_entries' LIMIT 1");
    if (legacy->HasError() || legacy->RowCount() != 0)
        return fail(error, QStringLiteral("legacy table retained"));

    auto nameCol = con.Query(
        "SELECT 1 FROM information_schema.columns "
        "WHERE table_schema='main' AND table_name='share_files' "
        "AND column_name='name' LIMIT 1");
    if (nameCol->HasError() || nameCol->RowCount() != 1)
        return fail(error, QStringLiteral("canonical name column missing"));

    auto counts = con.Query(
        "SELECT (SELECT count(*) FROM share_locations), "
        "(SELECT count(*) FROM share_files), (SELECT count(*) FROM share_users), "
        "(SELECT count(*) FROM share_list_meta)");
    if (counts->HasError() || counts->RowCount() != 1)
        return fail(error, QStringLiteral("migration counts query"));
    for (idx_t col = 0; col < 4; ++col) {
        if (ShareIndexDb::qi64(counts->GetValue(col, 0)) != 0)
            return fail(error, QStringLiteral("expected empty rebuild"));
    }
    return true;
}

} // namespace

/** Flat and pre-canonical DBs rebuild empty; lists re-ingest. */
bool shareIndexSmokeMigrate(const QString &path, QString *error)
{
    {
        duckdb::DuckDB db(path.toStdString());
        duckdb::Connection con(db);
        auto schema = con.Query(
            "CREATE TABLE share_entries (cid TEXT, hub_url TEXT, tth TEXT, path TEXT, "
            "name TEXT, ext TEXT, is_dir INTEGER, size BIGINT, nick TEXT, hub_name TEXT, "
            "free_slots INTEGER, all_slots INTEGER, ip TEXT, source INTEGER, "
            "created_at TEXT, name_cf TEXT, path_cf TEXT);"
            "CREATE TABLE share_list_meta (cid TEXT PRIMARY KEY, list_path TEXT, "
            "list_mtime BIGINT, list_size BIGINT, row_count INTEGER, indexed_at TEXT);"
            "INSERT INTO share_entries VALUES "
            "('A','adc://one','TTH1','P\\\\','One.bin','BIN',0,10,'Ann','Hub',1,2,'1',1,"
            "'1','one.bin','p\\\\');"
            "INSERT INTO share_list_meta VALUES ('A','list.xml',1,2,3,'4')");
        if (schema->HasError())
            return fail(error, QString::fromStdString(schema->GetError()));
    }
    {
        ShareIndex idx;
        idx.dbFile = path;
        idx.open();
        if (!checkEmptyRebuild(idx, *idx.threadConn(), error))
            return false;
    }

    const QString oldNorm = path + QStringLiteral(".oldnorm");
    {
        duckdb::DuckDB db(oldNorm.toStdString());
        duckdb::Connection con(db);
        auto schema = con.Query(
            "CREATE TABLE share_files ("
            "file_id BIGINT PRIMARY KEY, tth TEXT NOT NULL, size BIGINT NOT NULL, "
            "UNIQUE(tth, size));"
            "CREATE TABLE share_users ("
            "user_id BIGINT PRIMARY KEY, cid TEXT NOT NULL, hub_url TEXT NOT NULL, "
            "nick TEXT, hub_name TEXT, free_slots INTEGER, all_slots INTEGER, ip TEXT, "
            "updated_at TEXT, UNIQUE(cid, hub_url));"
            "CREATE TABLE share_locations ("
            "user_id BIGINT, file_id BIGINT, path TEXT, name TEXT, ext TEXT, "
            "is_dir INTEGER, local_size BIGINT, source INTEGER, created_at TEXT, "
            "name_cf TEXT, path_cf TEXT);"
            "INSERT INTO share_files VALUES (1,'OLD',1);"
            "INSERT INTO share_users VALUES (1,'U','h','n','H',NULL,NULL,'','t');"
            "INSERT INTO share_locations VALUES "
            "(1,1,'P\\\\','Old.bin','BIN',0,NULL,1,'t','old.bin','p\\\\')");
        if (schema->HasError())
            return fail(error, QString::fromStdString(schema->GetError()));
    }
    {
        ShareIndex idx;
        idx.dbFile = oldNorm;
        idx.open();
        if (!checkEmptyRebuild(idx, *idx.threadConn(), error))
            return false;
    }

    const QString metaOnly = path + QStringLiteral(".meta");
    {
        duckdb::DuckDB db(metaOnly.toStdString());
        duckdb::Connection con(db);
        auto schema = con.Query(
            "CREATE TABLE share_list_meta (cid TEXT PRIMARY KEY, list_path TEXT, "
            "list_mtime BIGINT, list_size BIGINT, row_count INTEGER, indexed_at TEXT);"
            "INSERT INTO share_list_meta VALUES ('STALE','list.xml',1,2,3,'4')");
        if (schema->HasError())
            return fail(error, QString::fromStdString(schema->GetError()));
    }
    {
        ShareIndex idx;
        idx.dbFile = metaOnly;
        idx.open();
        if (!checkEmptyRebuild(idx, *idx.threadConn(), error))
            return false;
    }

    ShareIndex idx;
    idx.dbFile = path;
    idx.open();
    if (!idx.isOpen())
        return fail(error, QStringLiteral("reopen: %1").arg(idx.lastError()));

    duckdb::Connection *con = idx.threadConn();
    QVariantMap row;
    row["cid"] = "C";
    row["hub_url"] = "adc://three";
    row["tth"] = "TTH3";
    row["path"] = "S\\";
    row["name"] = "Staged.bin";
    row["is_dir"] = false;
    row["size"] = 30;
    row["nick"] = "Cat";
    row["hub_name"] = "Hub";
    QList<QVariantMap> rows;
    rows << row;
    if (!idx.writeListRows(QStringLiteral("C"), rows))
        return fail(error, QStringLiteral("normalized bulk append"));

    auto staged = con->Query(
        "SELECT l.name IS NULL, f.name, f.path FROM share_locations l "
        "JOIN share_users u USING(user_id) JOIN share_files f USING(file_id) "
        "WHERE u.cid='C' AND f.tth='TTH3'");
    if (staged->HasError() || staged->RowCount() != 1
            || ShareIndexDb::qi64(staged->GetValue(0, 0)) != 1
            || ShareIndexDb::qstr(staged->GetValue(1, 0)) != QLatin1String("Staged.bin")
            || ShareIndexDb::qstr(staged->GetValue(2, 0)) != QLatin1String("S\\"))
        return fail(error, QStringLiteral("staged canonical / NULL override"));

    const ShareIndex::IndexStats stats = idx.indexStats();
    if (stats.files != 1 || stats.dbBytes <= 0)
        return fail(error, QStringLiteral("index stats missing"));

    idx.removeTthSync(QStringLiteral("C"), QStringLiteral("TTH3"));
    staged = con->Query("SELECT count(*) FROM share_files WHERE tth='TTH3'");
    if (staged->HasError() || ShareIndexDb::qi64(staged->GetValue(0, 0)) != 0)
        return fail(error, QStringLiteral("orphan file retained"));
    return true;
}

#endif
