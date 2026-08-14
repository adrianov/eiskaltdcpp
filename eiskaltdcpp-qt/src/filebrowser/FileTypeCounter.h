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

namespace SearchFileTypes {

/** Types present in one open file list (for the share-browser type combo). */
struct ListingTypes {
    bool hasDirs = false;
    bool hasAdultVideo = false;
    QVector<int> typeIds;
    QStringList customNames;
};

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

/** Any, then only types that exist (Audio & Video = Audio or Video; Adult Video after Video). */
void fillListingCombo(QComboBox *combo, const ListingTypes &types);

} // namespace SearchFileTypes
