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

/** Substring contains() checks for ShareIndex::smokeCheck. */
bool shareIndexSmokeSearch(ShareIndex &idx, duckdb::Connection &con, QString *error)
{
    ShareIndex::SearchFilter f;
    f.terms << "volume" << "softkey";

    QVariantMap soft;
    soft["cid"] = "CIDTEST";
    soft["hub_url"] = "adc://hub";
    soft["tth"] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234568";
    soft["path"] = "Apps\\";
    soft["name"] = "Hadcore Computing - Volume 1 (Softkey)";
    soft["is_dir"] = false;
    soft["size"] = 1;
    soft["nick"] = "Alice";
    soft["hub_name"] = "TestHub";
    if (!idx.upsertRow(con, soft, ShareIndex::SourceFileList)) {
        if (error)
            *error = "insert softkey";
        return false;
    }

    auto hits = idx.searchFts(con, f);
    if (hits.isEmpty()) {
        if (error)
            *error = QString("multi-term match: %1").arg(idx.lastError());
        return false;
    }

    f.terms.clear();
    f.terms << QString::fromUtf8("кабачок") << QString::fromUtf8("стульев");
    {
        QVariantMap cyr;
        cyr["cid"] = "CIDTEST";
        cyr["hub_url"] = "adc://hub";
        cyr["tth"] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234569";
        cyr["path"] = "TV\\";
        cyr["name"] = QString::fromUtf8("Кабачок.13.Стульев");
        cyr["is_dir"] = false;
        cyr["size"] = 2;
        cyr["nick"] = "Alice";
        cyr["hub_name"] = "TestHub";
        if (!idx.upsertRow(con, cyr, ShareIndex::SourceFileList)) {
            if (error)
                *error = "insert cyrillic";
            return false;
        }
    }
    hits = idx.searchFts(con, f);
    if (hits.isEmpty()) {
        if (error)
            *error = "cyrillic match";
        return false;
    }

    f.terms.clear();
    f.terms << "rare" << "historical" << "photos";
    {
        QVariantMap camel;
        camel["cid"] = "CIDTEST";
        camel["hub_url"] = "adc://hub";
        camel["tth"] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ23456A";
        camel["path"] = "Photos\\";
        camel["name"] = "RareHistoricalPhotosDotCom - sample.jpg";
        camel["is_dir"] = false;
        camel["size"] = 3;
        camel["nick"] = "Alice";
        camel["hub_name"] = "TestHub";
        if (!idx.upsertRow(con, camel, ShareIndex::SourceFileList)) {
            if (error)
                *error = "insert camel";
            return false;
        }
    }
    hits = idx.searchFts(con, f);
    if (hits.isEmpty()) {
        if (error)
            *error = "camelCase match";
        return false;
    }

    f.terms.clear();
    f.terms << QString::fromUtf8("жуков") << QString::fromUtf8("павлов");
    {
        QVariantMap ru;
        ru["cid"] = "CIDTEST";
        ru["hub_url"] = "adc://hub";
        ru["tth"] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ23456B";
        ru["path"] = "Video\\";
        ru["name"] = QString::fromUtf8("11 Как генерал Жуков разгромил генерала Павлова.mp4");
        ru["is_dir"] = false;
        ru["size"] = 4;
        ru["nick"] = "Alice";
        ru["hub_name"] = "TestHub";
        if (!idx.upsertRow(con, ru, ShareIndex::SourceFileList)) {
            if (error)
                *error = "insert ru";
            return false;
        }
    }
    hits = idx.searchFts(con, f);
    if (hits.isEmpty()) {
        if (error)
            *error = "ru inflection match";
        return false;
    }

    f.terms.clear();
    f.terms << "breaking";
    hits = idx.searchFts(con, f);
    if (hits.size() < 1) {
        if (error)
            *error = "match search";
        return false;
    }

    f.terms.clear();
    f.terms << "BREAKING";
    hits = idx.searchFts(con, f);
    if (hits.isEmpty()) {
        if (error)
            *error = "case search";
        return false;
    }

    f.terms.clear();
    f.terms << "S01E01";
    f.extensions << "MKV";
    hits = idx.searchFts(con, f);
    if (hits.isEmpty()) {
        if (error)
            *error = "ext filter";
        return false;
    }
    f.extensions.clear();
    f.extensions << "AVI";
    hits = idx.searchFts(con, f);
    if (!hits.isEmpty()) {
        if (error)
            *error = "ext filter false positive";
        return false;
    }

    f.extensions.clear();
    f.terms = QStringList() << "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    f.isHash = true;
    hits = idx.searchFts(con, f);
    if (hits.isEmpty() || hits.first().value("TTH").toString() != f.terms.first()) {
        if (error)
            *error = "TTH search";
        return false;
    }

    f.isHash = false;
    f.terms = QStringList() << "breaking";
    f.dirsOnly = true;
    hits = idx.searchFts(con, f);
    if (hits.isEmpty() || !hits.first().value("ISDIR").toBool()) {
        if (error)
            *error = "directory-only search";
        return false;
    }

    f.dirsOnly = false;
    f.filesOnly = true;
    hits = idx.searchFts(con, f);
    if (hits.isEmpty() || hits.first().value("ISDIR").toBool()) {
        if (error)
            *error = "file-only search";
        return false;
    }
    return true;
}

#endif
