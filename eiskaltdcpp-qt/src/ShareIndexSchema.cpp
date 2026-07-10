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

/** Normalized share-index schema. Names and paths stay scalar because each
    location is independently searchable and returned to the UI. */
bool ShareIndex::ensureSchema(duckdb::Connection &con, const std::string &prefix)
{
    QString err;
    if (!ShareIndexDb::execOk(con,
            "CREATE TABLE IF NOT EXISTS " + prefix + "share_files ("
            "file_id BIGINT PRIMARY KEY,"
            "tth TEXT NOT NULL,"
            "size BIGINT NOT NULL,"
            "UNIQUE(tth, size))", &err)
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
            "path TEXT NOT NULL,"
            "name TEXT NOT NULL,"
            "ext TEXT NOT NULL DEFAULT '',"
            "is_dir INTEGER NOT NULL DEFAULT 0,"
            "local_size BIGINT,"
            "source INTEGER NOT NULL,"
            "created_at TEXT NOT NULL,"
            "name_cf TEXT NOT NULL DEFAULT '',"
            "path_cf TEXT NOT NULL DEFAULT ''"
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
    return true;
}

#endif
