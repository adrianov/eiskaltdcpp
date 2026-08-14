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

#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

class QComboBox;

/** Shared SearchManager file-type combo and extension lists (SEGA / settings). */
namespace SearchFileTypes {

/**
 * Fill combo with predefined + custom types and icons.
 * When forSearch is false, omit Directory/TTH (useless for local ext filters).
 * Each item stores SearchManager type id in Qt::UserRole (TYPE_LAST for custom).
 */
void fillCombo(QComboBox *combo, bool forSearch = true);

/** True for the local Adult Video combo id (not a hub search type). */
bool isAdultVideoType(int typeIndex);

/**
 * Extensions for a SearchManager type index (uppercase, no leading dot).
 * Empty list means no extension filter (Any / Directory / TTH / unknown).
 * For custom types (index >= TYPE_LAST), pass the combo item text as typeName.
 * Adult Video uses the Video extension list.
 */
QStringList extensionsFor(int typeIndex, const QString &typeName = QString());

/** Types present in one open file list (for the share-browser type combo). */
struct ListingTypes {
    bool hasDirs = false;
    bool hasAdultVideo = false;
    QVector<int> typeIds;
    QStringList customNames;
};

/** Any, then only types that exist in the listing (Adult Video after Video). */
void fillListingCombo(QComboBox *combo, const ListingTypes &types);

/**
 * Counts files by search category (Audio, Video, … + custom; not Any/Directory/TTH/Audio&Video).
 * First matching type wins per file.
 */
class FileTypeCounter {
public:
    FileTypeCounter();
    void addFile(const QString &fileName, const QString &path = QString());
    /** Nonzero categories as "Video: 3; Audio: 1". */
    QString format() const;
    bool hasAdultVideo() const { return hasAdultVideo_; }
    void fillListing(ListingTypes &out) const;

private:
    struct Bucket {
        int typeId = 0;
        QString name;
        QStringList exts;
        int count = 0;
    };
    QVector<Bucket> buckets;
    QStringList videoExts_;
    bool hasAdultVideo_ = false;
};

} // namespace SearchFileTypes
