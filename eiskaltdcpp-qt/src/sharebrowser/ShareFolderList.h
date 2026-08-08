/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QString>

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

private:
    FileBrowserModel *model_;
    FileBrowserItem *root_;
    quint64 totalSize_ = 0;
    QString typeCounts_;
};
