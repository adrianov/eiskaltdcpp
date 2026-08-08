/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "ShareBrowser.h"
#include "FileBrowserModel.h"
#include "MediaEnrichQueue.h"
#include "WulforSettings.h"
#include "sharebrowser/ShareFolderList.h"

#include <QHeaderView>

using namespace dcpp;

void ShareBrowser::changeRoot(DirectoryListing::Directory *root)
{
    if (!folderList || !root)
        return;
    if (mediaEnrich)
        mediaEnrich->clearPending();
    folderList->showFolder(root);
    applyOptionalColumns();
    if (mediaEnrich)
        mediaEnrich->queue(folderList->missingMediaTths());
    label_RIGHT->setText(totalStatusText());
}

void ShareBrowser::changeRootFlat(DirectoryListing::Directory *root)
{
    if (!folderList || !root)
        return;
    if (mediaEnrich)
        mediaEnrich->clearPending();
    folderList->showFlat(listing, root);
    applyOptionalColumns();
    if (mediaEnrich)
        mediaEnrich->queue(folderList->missingMediaTths());
    label_RIGHT->setText(totalStatusText());
}

void ShareBrowser::applyOptionalColumns()
{
    if (!folderList || !treeView_RPANE)
        return;
    QHeaderView *h = treeView_RPANE->header();
    h->setSectionHidden(COLUMN_FILEBROWSER_BR, !folderList->hasBitrate());
    h->setSectionHidden(COLUMN_FILEBROWSER_WH, !folderList->hasResolution());
    h->setSectionHidden(COLUMN_FILEBROWSER_MVIDEO, !folderList->hasVideo());
    h->setSectionHidden(COLUMN_FILEBROWSER_MAUDIO, !folderList->hasAudio());
    h->setSectionHidden(COLUMN_FILEBROWSER_HIT, !folderList->hasDownloaded());
    h->setSectionHidden(COLUMN_FILEBROWSER_TS, !folderList->hasShared());
}

QString ShareBrowser::totalStatusText() const
{
    return folderList ? folderList->statusText() : QString();
}

void ShareBrowser::reloadRightPane(DirectoryListing::Directory *dir)
{
    if (!dir)
        return;
    if (flatMode)
        changeRootFlat(dir);
    else
        changeRoot(dir);
}

DirectoryListing::Directory *ShareBrowser::currentDir()
{
    if (treeView_LPANE && treeView_LPANE->selectionModel()) {
        const QModelIndexList selected = treeView_LPANE->selectionModel()->selectedRows(0);
        if (selected.size() == 1) {
            const QModelIndex index = treeMapToSource(selected.at(0));
            if (index.isValid()) {
                FileBrowserItem *item = static_cast<FileBrowserItem*>(index.internalPointer());
                if (item && item->dir)
                    return item->dir;
            }
        }
    }

    if (!lineEdit_PATH->text().isEmpty()) {
        FileBrowserItem *item = tree_model->createRootForPath(lineEdit_PATH->text());
        if (item && item->dir)
            return item->dir;
    }

    return listing.getRoot();
}

void ShareBrowser::applyFlatMode(bool on)
{
    flatMode = on;
    frame_2->setVisible(!on);
    toolButton_BACK->setEnabled(!on);
    toolButton_FORWARD->setEnabled(!on);
    toolButton_UP->setEnabled(!on);
    treeView_RPANE->header()->setSectionHidden(COLUMN_FILEBROWSER_PATH, !on);
    reloadRightPane(currentDir());
    applyViewFiltersNow();
}

void ShareBrowser::slotFlatToggled(bool on)
{
    WBSET(WB_SHARE_FLAT, on);
    applyFlatMode(on);
}
