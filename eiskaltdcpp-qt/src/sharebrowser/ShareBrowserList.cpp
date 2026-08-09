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
    if (lineEdit_PATH->text().isEmpty()) {
        toolButton_UP->setEnabled(false);
        return;
    }
    FileBrowserItem *item = tree_model->createRootForPath(lineEdit_PATH->text());
    toolButton_UP->setEnabled(item && item->parent() && item->parent()->dir);
}

void ShareBrowser::restoreSplitterSizes()
{
    constexpr int kDefaultTreeWidth = 240; // UIShareBrowser frame_2 baseSize
    constexpr int kMinRight = 160;         // UIShareBrowser frame minimumSize
    constexpr int kMinLeft = 80;
    const int total = qMax(splitter->width(), kDefaultTreeWidth + kMinRight);
    const int w = WIGET(WI_SHARE_WIDTH);
    const int wr = WIGET(WI_SHARE_RPANE_WIDTH);
    int left = (w > 0 && wr > 0 && wr < w) ? (w - wr) : 0;
    // 0 (or tiny) left happens after starting in Flat — fall back to default.
    if (left < kMinLeft)
        left = kDefaultTreeWidth;
    // Keep a wide folder pane when the window is temporarily narrower than saved.
    if (left > total - kMinRight)
        left = qMax(kMinLeft, total - kMinRight);
    splitter->setSizes(QList<int>() << left << qMax(total - left, 1));
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
    if (!on)
        restoreSplitterSizes();

    toolButton_BACK->setEnabled(!on);
    toolButton_FORWARD->setEnabled(!on);
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
