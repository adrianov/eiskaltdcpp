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

bool ShareIndex::ensureSchema(QSqlDatabase &db)
{
    QSqlQuery q(db);
    if (!q.exec(
            "CREATE TABLE IF NOT EXISTS share_entries ("
            "id INTEGER PRIMARY KEY,"
            "cid TEXT NOT NULL,"
            "hub_url TEXT NOT NULL DEFAULT '',"
            "tth TEXT NOT NULL DEFAULT '',"
            "path TEXT NOT NULL,"
            "name TEXT NOT NULL,"
            "ext TEXT NOT NULL DEFAULT '',"
            "is_dir INTEGER NOT NULL DEFAULT 0,"
            "size INTEGER NOT NULL DEFAULT 0,"
            "nick TEXT NOT NULL DEFAULT '',"
            "hub_name TEXT NOT NULL DEFAULT '',"
            "free_slots INTEGER,"
            "all_slots INTEGER,"
            "ip TEXT,"
            "bitrate INTEGER,"
            "resolution TEXT,"
            "video_info TEXT,"
            "audio_info TEXT,"
            "hit INTEGER,"
            "show_hits INTEGER NOT NULL DEFAULT 0,"
            "shared_ts INTEGER,"
            "source INTEGER NOT NULL,"
            "created_at TEXT NOT NULL,"
            "updated_at TEXT NOT NULL"
            ")"))
        return false;

    // Migrate DBs created before ext existed.
    q.exec("ALTER TABLE share_entries ADD COLUMN ext TEXT NOT NULL DEFAULT ''");
    // Unicode case-fold copies for FTS trigram (and any LIKE tooling).
    q.exec("ALTER TABLE share_entries ADD COLUMN name_cf TEXT NOT NULL DEFAULT ''");
    q.exec("ALTER TABLE share_entries ADD COLUMN path_cf TEXT NOT NULL DEFAULT ''");
    /** Times this row was shown from local Search (not remote file-list HIT). */
    q.exec("ALTER TABLE share_entries ADD COLUMN show_hits INTEGER NOT NULL DEFAULT 0");

    q.exec("CREATE UNIQUE INDEX IF NOT EXISTS share_entries_file_tth "
           "ON share_entries(cid, tth) WHERE is_dir = 0 AND tth != ''");
    q.exec("CREATE UNIQUE INDEX IF NOT EXISTS share_entries_path_name "
           "ON share_entries(cid, path, name, is_dir) WHERE is_dir = 1 OR tth = ''");
    q.exec("CREATE INDEX IF NOT EXISTS share_entries_cid ON share_entries(cid)");
    q.exec("CREATE INDEX IF NOT EXISTS share_entries_updated ON share_entries(updated_at)");
    q.exec("CREATE INDEX IF NOT EXISTS share_entries_ext ON share_entries(ext)");
    // Cap eviction: lowest show_hits, then oldest updated_at / created_at.
    q.exec("CREATE INDEX IF NOT EXISTS share_entries_evict "
           "ON share_entries(show_hits, updated_at, created_at)");

    // Skip re-indexing a file list when path/mtime/size are unchanged.
    q.exec("CREATE TABLE IF NOT EXISTS share_list_meta ("
           "cid TEXT PRIMARY KEY,"
           "list_path TEXT NOT NULL,"
           "list_mtime INTEGER NOT NULL,"
           "list_size INTEGER NOT NULL,"
           "row_count INTEGER NOT NULL DEFAULT 0,"
           "indexed_at TEXT NOT NULL)");
    return true;
}

#endif
