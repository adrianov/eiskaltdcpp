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

    const QModelIndex openIdx = tree_model->createIndexForItem(item);
    for (QModelIndex p = openIdx.parent(); p.isValid(); p = p.parent())
        treeView_LPANE->expand(p);

    // ClearAndSelect replaces any prior rows. SelectCurrent only adds, and
    // slotLeftPaneSelChanged ignores multi-select — so Browse folder used to
    // highlight the jump path while the right pane stayed on the previous root.
    treeView_LPANE->selectionModel()->setCurrentIndex(openIdx,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    treeView_LPANE->scrollTo(openIdx, QAbstractItemView::PositionAtCenter);
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
