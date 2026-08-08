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
#include <QVariantMap>

#include "sharebrowser/ShareMediaState.h"

#include "dcpp/stdinc.h"
#include "dcpp/DirectoryListing.h"

class FileBrowserModel;
class FileBrowserItem;

/**
 * Right-pane contents for a share listing: one folder level or a flat file list,
 * plus total size and per-type file counts for the status line.
 */
class ShareFolderList {
public:
    ShareFolderList(FileBrowserModel *model, FileBrowserItem *root);

    void showFolder(dcpp::DirectoryListing::Directory *dir);
    void showFlat(dcpp::DirectoryListing &listing, dcpp::DirectoryListing::Directory *dir);

    quint64 totalSize() const { return totalSize_; }
    /** "Total size: …; Video: 3; Audio: 1" (type counts only when nonzero). */
    QString statusText() const;

    bool hasBitrate() const { return media_.hasBitrate(); }
    bool hasResolution() const { return media_.hasResolution(); }
    bool hasVideo() const { return media_.hasVideo(); }
    bool hasAudio() const { return media_.hasAudio(); }
    bool hasDownloaded() const { return media_.hasDownloaded(); }
    bool hasShared() const { return media_.hasShared(); }

    QStringList missingMediaTths() const { return media_.missingTths(); }
    void noteMedia(const QVariantMap &m) { media_.noteEnrich(m); }

private:
    FileBrowserModel *model_;
    FileBrowserItem *root_;
    ShareMediaState media_;
    quint64 totalSize_ = 0;
    QString typeCounts_;
};
