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

#include "ShareBrowser.h"
#include "FileBrowserModel.h"
#include "MediaEnrichQueue.h"
#include "WulforSettings.h"
#include "WulforUtil.h"
#include "sharebrowser/ShareFolderList.h"
#include "sharebrowser/ShareListColumns.h"

#include <QHeaderView>

using namespace dcpp;

void ShareBrowser::changeRoot(DirectoryListing::Directory *root)
{
    loadRoot(root, false);
}

void ShareBrowser::changeRootFlat(DirectoryListing::Directory *root)
{
    loadRoot(root, true);
}

void ShareBrowser::loadRoot(DirectoryListing::Directory *root, bool flat)
{
    if (!folderList || !root)
        return;
    if (mediaEnrich)
        mediaEnrich->clearPending();
    if (flat)
        folderList->showFlat(listing, root);
    else
        folderList->showFolder(root);
    applyOptionalColumns();
    if (mediaEnrich)
        mediaEnrich->queue(folderList->missingMediaTths());
    label_RIGHT->setText(totalStatusText());
}

void ShareBrowser::applyOptionalColumns()
{
    if (!folderList || !treeView_RPANE)
        return;
    // Re-fit only when optional columns appear/disappear — not on every media batch
    // (that kept TreeHeaderAutosize dirty and stalled main side-dock resizing).
    if (ShareListColumns(treeView_RPANE->header()).apply(*folderList))
        WulforUtil::ensureTreeHeaderAutosized(treeView_RPANE);
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
    // Path bar matches the shown listing; tree selection may be multi/empty/stale.
    if (!lineEdit_PATH->text().isEmpty()) {
        FileBrowserItem *item = tree_model->createRootForPath(lineEdit_PATH->text());
        if (item && item->dir)
            return item->dir;
    }

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

    return listing.getRoot();
}

void ShareBrowser::updateUpButton()
{
    FileBrowserItem *item = nullptr;
    if (!lineEdit_PATH->text().isEmpty())
        item = tree_model->createRootForPath(lineEdit_PATH->text());

    toolButton_UP->setEnabled(item && item->parent() && item->parent()->dir);

    // Flat: Prev/Next walk sibling folders. Folder view: history (always enabled).
    if (flatMode) {
        toolButton_BACK->setEnabled(item && item->prevSibling());
        toolButton_FORWARD->setEnabled(item && item->nextSibling());
    } else {
        toolButton_BACK->setEnabled(true);
        toolButton_FORWARD->setEnabled(true);
    }
}

void ShareBrowser::restoreSplitterSizes()
{
    // Defer until the splitter has a real width (HubPaneLayout does the same).
    // Applying against a tiny pre-layout width + stretch used to grow the folder pane.
    if (flatMode || splitReady)
        return;

    constexpr int kMinRight = 160; // UIShareBrowser frame minimumSize
    constexpr int kMinLeft = 80;
    const int total = splitter->width();
    if (total < kMinLeft + kMinRight)
        return;

    const int w = WIGET(WI_SHARE_WIDTH);
    const int wr = WIGET(WI_SHARE_RPANE_WIDTH);
    const bool haveSaved = (w > 0 && wr > 0 && wr < w);
    // Saved folder width, or default left ≈ half the right pane (1:2).
    int left = haveSaved ? (w - wr) : (total / 3);
    left = qBound(kMinLeft, left, total - kMinRight);

    // Saved: keep folder width sticky. Default 1:2: keep the ratio on resize.
    splitter->setStretchFactor(0, haveSaved ? 0 : 1);
    splitter->setStretchFactor(1, haveSaved ? 1 : 2);
    splitReady = true;
    splitter->setSizes(QList<int>() << left << (total - left));
}

void ShareBrowser::applyFlatMode(bool on)
{
    // QSplitter keeps a hidden pane at width 0; save/restore so un-flat shows the tree again.
    if (on && !flatMode) {
        const QList<int> sizes = splitter->sizes();
        if (sizes.size() >= 2 && sizes.at(0) >= 80) {
            WISET(WI_SHARE_RPANE_WIDTH, sizes.at(1));
            WISET(WI_SHARE_WIDTH, sizes.at(0) + sizes.at(1));
        }
    }

    flatMode = on;
    frame_2->setVisible(!on);
    if (!on) {
        splitReady = false;
        restoreSplitterSizes();
    }

    updateUpButton();

    ShareListColumns(treeView_RPANE->header()).setPathVisible(on);
    WulforUtil::ensureTreeHeaderAutosized(treeView_RPANE);

    // Flat default is Path; folder default is Name. Keep Size/etc. if the user chose it.
    const int cur = list_model->getSortColumn();
    const int defCol = on ? COLUMN_FILEBROWSER_PATH : COLUMN_FILEBROWSER_NAME;
    if (cur == COLUMN_FILEBROWSER_NAME || cur == COLUMN_FILEBROWSER_PATH) {
        list_model->setSortColumn(defCol);
        list_model->setSortOrder(Qt::AscendingOrder);
        treeView_RPANE->header()->setSortIndicator(defCol, Qt::AscendingOrder);
    }

    reloadRightPane(currentDir());
    applyViewFiltersNow();
}

void ShareBrowser::slotFlatToggled(bool on)
{
    WBSET(WB_SHARE_FLAT, on);
    applyFlatMode(on);
}
