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

bool ShareIndex::ensureFts(QSqlDatabase &db)
{
    QSqlQuery q(db);

    // Prefer FTS5 trigram for case-independent substring match.
    if (!q.exec(
            "CREATE VIRTUAL TABLE IF NOT EXISTS share_entries_fts USING fts5("
            "name, path, content='share_entries', content_rowid='id', tokenize='trigram')")) {
        // Older SQLite / no trigram: try unicode61 FTS5, then give up (LIKE fallback).
        if (!q.exec(
                "CREATE VIRTUAL TABLE IF NOT EXISTS share_entries_fts USING fts5("
                "name, path, content='share_entries', content_rowid='id')"))
            return false;
    }

    q.exec(
        "CREATE TRIGGER IF NOT EXISTS share_entries_ai AFTER INSERT ON share_entries BEGIN "
        "INSERT INTO share_entries_fts(rowid, name, path) VALUES (new.id, new.name, new.path); "
        "END");
    q.exec(
        "CREATE TRIGGER IF NOT EXISTS share_entries_ad AFTER DELETE ON share_entries BEGIN "
        "INSERT INTO share_entries_fts(share_entries_fts, rowid, name, path) "
        "VALUES('delete', old.id, old.name, old.path); "
        "END");
    q.exec(
        "CREATE TRIGGER IF NOT EXISTS share_entries_au AFTER UPDATE ON share_entries BEGIN "
        "INSERT INTO share_entries_fts(share_entries_fts, rowid, name, path) "
        "VALUES('delete', old.id, old.name, old.path); "
        "INSERT INTO share_entries_fts(rowid, name, path) VALUES (new.id, new.name, new.path); "
        "END");

    // Rebuild FTS from content if empty (first create after existing rows).
    QSqlQuery cnt(db);
    if (cnt.exec("SELECT count(*) FROM share_entries_fts") && cnt.next() && cnt.value(0).toLongLong() == 0) {
        QSqlQuery fill(db);
        if (fill.exec("SELECT count(*) FROM share_entries") && fill.next() && fill.value(0).toLongLong() > 0)
            q.exec("INSERT INTO share_entries_fts(share_entries_fts) VALUES('rebuild')");
    }

    return true;
}

#endif
