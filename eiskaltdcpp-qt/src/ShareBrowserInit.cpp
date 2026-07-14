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
#include "ArenaWidgetManager.h"

#include "dcpp/ClientManager.h"

#include <QAbstractItemView>

using namespace dcpp;

void ShareBrowser::selectLeftFolder(FileBrowserItem *item){
    if (!item)
        return;

    const QModelIndex src = tree_model->createIndexForItem(item);
    const QModelIndex viewIdx = treeMapFromSource(src);
    for (QModelIndex p = viewIdx.parent(); p.isValid(); p = p.parent())
        treeView_LPANE->expand(p);

    treeView_LPANE->selectionModel()->setCurrentIndex(viewIdx,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    treeView_LPANE->scrollTo(viewIdx, QAbstractItemView::PositionAtCenter);
}

void ShareBrowser::continueInit(){
    tree_model->repaint();
    list_model->repaint();

    load();

    if (user == ClientManager::getInstance()->getMe()){
        tree_model->loadRestrictions();
        list_model->setOwnList(true);
    }

    // Prefer jump_to (Browse folder); else list root so flat lists show files immediately.
    FileBrowserItem *openItem = nullptr;
    if (!jump_to.isEmpty()) {
        FileBrowserItem *shareRoot = tree_model->getRootElem();
        if (shareRoot && !shareRoot->childItems.isEmpty())
            openItem = tree_model->createRootForPath(jump_to, shareRoot->childItems.at(0));
    }
    if (!openItem) {
        const QModelIndex rootIdx = tree_model->index(0, 0);
        if (rootIdx.isValid())
            openItem = static_cast<FileBrowserItem*>(rootIdx.internalPointer());
    }
    selectLeftFolder(openItem);

    pathHistory.clear();

    ArenaWidgetManager::getInstance()->activate(this);
}
