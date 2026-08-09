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

#include <QHeaderView>

using namespace dcpp;

namespace {

void placeAfter(QHeaderView *h, int afterCol, int moveCol)
{
    if (!h || h->isSectionHidden(moveCol) || h->isSectionHidden(afterCol))
        return;
    const int afterVis = h->visualIndex(afterCol);
    const int moveVis = h->visualIndex(moveCol);
    if (afterVis < 0 || moveVis < 0)
        return;
    // When moveCol is before afterCol, target is afterVis (removal shifts left).
    const int to = afterVis + (moveVis > afterVis ? 1 : 0);
    if (moveVis != to)
        h->moveSection(moveVis, to);
}

void applyColumnOrder(QHeaderView *h)
{
    placeAfter(h, COLUMN_FILEBROWSER_NAME, COLUMN_FILEBROWSER_PATH);
    placeAfter(h, COLUMN_FILEBROWSER_SIZE, COLUMN_FILEBROWSER_WH);
}

} // namespace

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
    bool changed = false;
    const auto setHidden = [&](int col, bool hidden) {
        if (h->isSectionHidden(col) != hidden) {
            h->setSectionHidden(col, hidden);
            changed = true;
        }
    };
    setHidden(COLUMN_FILEBROWSER_BR, !folderList->hasBitrate());
    setHidden(COLUMN_FILEBROWSER_WH, !folderList->hasResolution());
    setHidden(COLUMN_FILEBROWSER_MVIDEO, !folderList->hasVideo());
    setHidden(COLUMN_FILEBROWSER_MAUDIO, !folderList->hasAudio());
    setHidden(COLUMN_FILEBROWSER_HIT, !folderList->hasDownloaded());
    setHidden(COLUMN_FILEBROWSER_TS, !folderList->hasShared());
    applyColumnOrder(h);
    // Re-fit only when optional columns appear/disappear — not on every media batch
    // (that kept TreeHeaderAutosize dirty and stalled main side-dock resizing).
    if (changed)
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
    const int total = qMax(splitter->width(), kDefaultTreeWidth + 100);
    const int w = WIGET(WI_SHARE_WIDTH);
    const int wr = WIGET(WI_SHARE_RPANE_WIDTH);
    int left = (w > 0 && wr > 0 && wr < w) ? (w - wr) : 0;
    // 0 (or tiny) left happens after starting in Flat — fall back to default.
    if (left < 80)
        left = kDefaultTreeWidth;
    if (left > total - 100)
        left = qMin(kDefaultTreeWidth, total - 100);
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

    treeView_RPANE->header()->setSectionHidden(COLUMN_FILEBROWSER_PATH, !on);
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
