/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ShareIndex.h"

#ifdef USE_QT_SQLITE

/** Files keep first-found name/path; locations store NULL when equal. */
bool ShareIndex::ensureSchema(duckdb::Connection &con, const std::string &prefix)
{
    QString err;
    if (!ShareIndexDb::execOk(con,
            "CREATE TABLE IF NOT EXISTS " + prefix + "share_files ("
            "file_id BIGINT PRIMARY KEY,"
            "tth TEXT NOT NULL,"
            "size BIGINT NOT NULL,"
            "name TEXT NOT NULL,"
            "path TEXT NOT NULL,"
            "ext TEXT NOT NULL DEFAULT '',"
            "name_cf TEXT NOT NULL,"
            "path_cf TEXT NOT NULL,"
            "bitrate INTEGER,"
            "resolution TEXT,"
            "video TEXT,"
            "audio TEXT,"
            "UNIQUE(tth))", &err)
            || !ShareIndexDb::execOk(con,
            "CREATE TABLE IF NOT EXISTS " + prefix + "share_users ("
            "user_id BIGINT PRIMARY KEY,"
            "cid TEXT NOT NULL,"
            "hub_url TEXT NOT NULL DEFAULT '',"
            "nick TEXT NOT NULL DEFAULT '',"
            "hub_name TEXT NOT NULL DEFAULT '',"
            "free_slots INTEGER,"
            "all_slots INTEGER,"
            "ip TEXT,"
            "updated_at TEXT NOT NULL,"
            "UNIQUE(cid, hub_url))", &err)
            || !ShareIndexDb::execOk(con,
            "CREATE TABLE IF NOT EXISTS " + prefix + "share_locations ("
            "user_id BIGINT NOT NULL,"
            "file_id BIGINT,"
            "path TEXT,"
            "name TEXT,"
            "ext TEXT,"
            "is_dir INTEGER NOT NULL DEFAULT 0,"
            "local_size BIGINT,"
            "source INTEGER NOT NULL,"
            "created_at TEXT NOT NULL,"
            "name_cf TEXT,"
            "path_cf TEXT"
            ")", &err)) {
        setLastError(err);
        return false;
    }

    ShareIndexDb::execOk(con,
            "CREATE INDEX IF NOT EXISTS share_users_cid ON "
            + prefix + "share_users(cid)");

    if (!ShareIndexDb::execOk(con,
            "CREATE TABLE IF NOT EXISTS " + prefix + "share_list_meta ("
            "cid TEXT PRIMARY KEY,"
            "list_path TEXT NOT NULL,"
            "list_mtime BIGINT NOT NULL,"
            "list_size BIGINT NOT NULL,"
            "row_count INTEGER NOT NULL DEFAULT 0,"
            "indexed_at TEXT NOT NULL)", &err)) {
        setLastError(err);
        return false;
    }

    // Existing DBs created before media columns (nullable; empty means NULL).
    static const char *mediaCols[] = {
        "bitrate INTEGER",
        "resolution TEXT",
        "video TEXT",
        "audio TEXT"
    };
    const QString table = QString::fromStdString(prefix) + QStringLiteral("share_files");
    for (const char *col : mediaCols) {
        if (!ShareIndexDb::execOk(con,
                QStringLiteral("ALTER TABLE %1 ADD COLUMN IF NOT EXISTS %2")
                    .arg(table, QString::fromLatin1(col)).toStdString(),
                &err)) {
            setLastError(err);
            return false;
        }
    }
    // Older creates used NOT NULL DEFAULT 0/''; allow NULL for empty media.
    static const char *mediaNull[] = { "bitrate", "resolution", "video", "audio" };
    for (const char *col : mediaNull)
        ShareIndexDb::execOk(con,
                QStringLiteral("ALTER TABLE %1 ALTER COLUMN %2 DROP NOT NULL")
                    .arg(table, QString::fromLatin1(col)).toStdString());
    return true;
}

#endif
