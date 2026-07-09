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

bool ftsHasCaseFold(QSqlDatabase &db)
{
    QSqlQuery q(db);
    if (!q.exec("PRAGMA table_info(share_entries_fts)"))
        return false;
    while (q.next()) {
        if (q.value(1).toString() == QLatin1String("name_cf"))
            return true;
    }
    return false;
}

void dropFts(QSqlDatabase &db)
{
    QSqlQuery q(db);
    q.exec("DROP TRIGGER IF EXISTS share_entries_ai");
    q.exec("DROP TRIGGER IF EXISTS share_entries_ad");
    q.exec("DROP TRIGGER IF EXISTS share_entries_au");
    q.exec("DROP TABLE IF EXISTS share_entries_fts");
}

void rebuildFts(QSqlDatabase &db)
{
    QSqlQuery q(db);
    q.exec("INSERT INTO share_entries_fts(share_entries_fts) VALUES('rebuild')");
}

} // namespace

bool ShareIndex::ensureFts(QSqlDatabase &db)
{
    QSqlQuery q(db);
    bool migrated = false;

    // Old FTS indexed raw name/path; rebuild on case-folded columns.
    if (q.exec("SELECT 1 FROM sqlite_master WHERE type='table' AND name='share_entries_fts'")
            && q.next() && !ftsHasCaseFold(db)) {
        dropFts(db);
        migrated = true;
    }

    // Trigram FTS on case-folded columns: MATCH does unordered substring AND.
    if (!q.exec(
            "CREATE VIRTUAL TABLE IF NOT EXISTS share_entries_fts USING fts5("
            "name_cf, path_cf, content='share_entries', content_rowid='id', "
            "tokenize='trigram')")) {
        setLastError(q.lastError().text());
        return false;
    }

    q.exec(
        "CREATE TRIGGER IF NOT EXISTS share_entries_ai AFTER INSERT ON share_entries BEGIN "
        "INSERT INTO share_entries_fts(rowid, name_cf, path_cf) "
        "VALUES (new.id, new.name_cf, new.path_cf); "
        "END");
    q.exec(
        "CREATE TRIGGER IF NOT EXISTS share_entries_ad AFTER DELETE ON share_entries BEGIN "
        "INSERT INTO share_entries_fts(share_entries_fts, rowid, name_cf, path_cf) "
        "VALUES('delete', old.id, old.name_cf, old.path_cf); "
        "END");
    q.exec(
        "CREATE TRIGGER IF NOT EXISTS share_entries_au AFTER UPDATE ON share_entries BEGIN "
        "INSERT INTO share_entries_fts(share_entries_fts, rowid, name_cf, path_cf) "
        "VALUES('delete', old.id, old.name_cf, old.path_cf); "
        "INSERT INTO share_entries_fts(rowid, name_cf, path_cf) "
        "VALUES (new.id, new.name_cf, new.path_cf); "
        "END");

    // Avoid count(*) / MATCH probes on multi-GB DBs (WAL checkpoint / UI hang).
    // Rebuild only when the FTS schema itself changed.
    if (migrated)
        rebuildFts(db);

    return true;
}

#endif
