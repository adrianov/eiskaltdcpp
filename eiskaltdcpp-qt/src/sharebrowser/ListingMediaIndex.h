/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QHash>
#include <QString>

#include "ShareIndex.h"

#include "dcpp/stdinc.h"
#include "dcpp/DirectoryListing.h"
#include "dcpp/User.h"

/**
 * Collects media fields from a loaded file list and publishes them to ShareIndex
 * so Search can enrich the same TTHs (including files already in the local share).
 */
class ListingMediaIndex {
public:
    void collectFrom(dcpp::DirectoryListing::Directory *root);
    /** Queue list ingest (may skip on mtime) plus media upsert for collected TTHs. */
    void publish(const dcpp::UserPtr &user, const QString &listPath, const QString &nick);
    bool isEmpty() const { return media_.isEmpty(); }

private:
    void collectDir(dcpp::DirectoryListing::Directory *dir);

    QHash<QString, ShareIndex::MediaInfo> media_;
};
