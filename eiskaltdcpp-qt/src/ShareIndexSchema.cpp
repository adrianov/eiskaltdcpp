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

bool ShareIndex::ensureSchema(duckdb::Connection &con)
{
    QString err;
    if (!ShareIndexDb::execOk(con,
            "CREATE SEQUENCE IF NOT EXISTS share_entry_seq START 1", &err)) {
        setLastError(err);
        return false;
    }

    // No hit / show_hits / updated_at. created_at kept for age trim.
    // ukey replaces partial unique indexes (DuckDB-friendly upsert key).
    if (!ShareIndexDb::execOk(con,
            "CREATE TABLE IF NOT EXISTS share_entries ("
            "id BIGINT PRIMARY KEY DEFAULT nextval('share_entry_seq'),"
            "ukey TEXT NOT NULL UNIQUE,"
            "cid TEXT NOT NULL,"
            "hub_url TEXT NOT NULL DEFAULT '',"
            "tth TEXT NOT NULL DEFAULT '',"
            "path TEXT NOT NULL,"
            "name TEXT NOT NULL,"
            "ext TEXT NOT NULL DEFAULT '',"
            "is_dir INTEGER NOT NULL DEFAULT 0,"
            "size BIGINT NOT NULL DEFAULT 0,"
            "nick TEXT NOT NULL DEFAULT '',"
            "hub_name TEXT NOT NULL DEFAULT '',"
            "free_slots INTEGER,"
            "all_slots INTEGER,"
            "ip TEXT,"
            "bitrate INTEGER,"
            "resolution TEXT,"
            "video_info TEXT,"
            "audio_info TEXT,"
            "shared_ts BIGINT,"
            "source INTEGER NOT NULL,"
            "created_at TEXT NOT NULL,"
            "name_cf TEXT NOT NULL DEFAULT '',"
            "path_cf TEXT NOT NULL DEFAULT ''"
            ")", &err)) {
        setLastError(err);
        return false;
    }

    ShareIndexDb::execOk(con, "CREATE INDEX IF NOT EXISTS share_entries_cid ON share_entries(cid)");
    ShareIndexDb::execOk(con, "CREATE INDEX IF NOT EXISTS share_entries_ext ON share_entries(ext)");
    // Trim / cap eviction by age only (no show_hits / updated_at index).
    ShareIndexDb::execOk(con,
            "CREATE INDEX IF NOT EXISTS share_entries_created ON share_entries(created_at)");

    if (!ShareIndexDb::execOk(con,
            "CREATE TABLE IF NOT EXISTS share_list_meta ("
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
