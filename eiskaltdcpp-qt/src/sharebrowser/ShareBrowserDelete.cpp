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
#include "sharebrowser/ShareOwnDelete.h"

#include "dcpp/ClientManager.h"

using namespace dcpp;

void ShareBrowser::refreshAfterOwnDelete(DirectoryListing::Directory *viewParent, bool removedDir)
{
    listing.rebuildTthIndex();
    if (removedDir) {
        pathHistory.clear();
        pathHistory_iter = pathHistory.end();
        tree_model->clear();
        tree_model->fetchMore(QModelIndex());
        const QModelIndex rootIdx = treeMapFromSource(tree_model->index(0, 0));
        if (rootIdx.isValid())
            treeView_LPANE->setExpanded(rootIdx, true);
    }

    if (!viewParent)
        return;

    if (removedDir) {
        FileBrowserItem *listRoot = tree_model->getRootElem()->child(0);
        QString remote = _q(listing.getPath(viewParent));
        while (remote.endsWith(QLatin1Char('\\')))
            remote.chop(1);

        FileBrowserItem *ti = remote.isEmpty() ? listRoot
            : tree_model->createRootForPath(remote, listRoot);
        if (ti) {
            if (flatMode) {
                goToFlatItem(ti);
                return;
            }
            const QModelIndex ix = treeMapFromSource(tree_model->createIndexForItem(ti));
            treeView_LPANE->selectionModel()->select(ix,
                    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            return;
        }
    }

    reloadRightPane(flatMode ? currentDir() : viewParent);
    updateUpButton();
}

void ShareBrowser::deleteOwnItems(const QModelIndexList &list)
{
    if (user != ClientManager::getInstance()->getMe())
        return;
    refreshAfterOwnDelete(ShareOwnDelete(listing).deleteFiles(list), false);
}

void ShareBrowser::deleteOwnOtherCopies(const QModelIndexList &list)
{
    if (user != ClientManager::getInstance()->getMe())
        return;
    refreshAfterOwnDelete(ShareOwnDelete(listing).deleteOtherCopies(list), false);
}

void ShareBrowser::deleteOwnWholeDir(const QModelIndexList &list)
{
    if (user != ClientManager::getInstance()->getMe())
        return;
    const auto result = ShareOwnDelete(listing).deleteDirs(list);
    refreshAfterOwnDelete(result.viewParent, result.removedDir);
}
