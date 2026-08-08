/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "sharebrowser/ListingMediaIndex.h"
#include "WulforUtil.h"

using namespace dcpp;

void ListingMediaIndex::collectFrom(DirectoryListing::Directory *root)
{
    media_.clear();
    collectDir(root);
}

void ListingMediaIndex::collectDir(DirectoryListing::Directory *dir)
{
    if (!dir)
        return;
    for (const auto &file : dir->files) {
        const MediaInfo &mi = file->mediaInfo;
        if (mi.bitrate == 0 && mi.resolution.empty()
                && mi.video_info.empty() && mi.audio_info.empty())
            continue;
        const QString tth = _q(file->getTTH().toBase32());
        if (tth.isEmpty() || media_.contains(tth))
            continue;
        ShareIndex::MediaInfo m;
        m.bitrate = int(mi.bitrate);
        m.resolution = _q(mi.resolution);
        m.video = _q(mi.video_info);
        m.audio = _q(mi.audio_info);
        media_.insert(tth, m);
    }
    for (const auto &sub : dir->directories)
        collectDir(sub);
}

void ListingMediaIndex::publish(const UserPtr &user, const QString &listPath, const QString &nick)
{
    ShareIndex *idx = ShareIndex::getInstance();
    if (!idx || !user || listPath.isEmpty())
        return;
    // Full ingest may skip when list mtime matches; upsert still fills empty media.
    idx->ingestList(user, listPath, QString(), nick);
    if (!media_.isEmpty())
        idx->upsertMedia(media_);
}
