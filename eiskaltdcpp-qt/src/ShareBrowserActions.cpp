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
#include "WulforUtil.h"
#include "FileBrowserModel.h"
#include "MainWindow.h"
#include "ArenaWidgetManager.h"
#include "sharebrowser/ShareListColumns.h"

#include "dcpp/FavoriteManager.h"
#include "dcpp/ClientManager.h"

#include <QMessageBox>
#include <QSignalBlocker>

using namespace dcpp;

void ShareBrowser::goToFlatItem(FileBrowserItem *item)
{
    if (!item || !item->dir)
        return;

    QItemSelectionModel *selection_model = treeView_LPANE->selectionModel();
    const QModelIndex src = tree_model->createIndexForItem(item);
    QModelIndex viewIdx = treeMapFromSource(src);
    {
        const QSignalBlocker block(selection_model);
        if (viewIdx.isValid()) {
            for (QModelIndex p = viewIdx.parent(); p.isValid(); p = p.parent())
                treeView_LPANE->expand(p);
            selection_model->setCurrentIndex(viewIdx,
                    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        }
    }
    lineEdit_PATH->setText(tree_model->createRemotePath(item));
    changeRootFlat(item->dir);
    applyViewFiltersNow();
    updateUpButton();
}

void ShareBrowser::slotButtonUp(){
    // Path bar is the source of truth — ignore tree multi-select / empty selection.
    if (lineEdit_PATH->text().isEmpty())
        return;

    FileBrowserItem *item = tree_model->createRootForPath(lineEdit_PATH->text());
    if (!item || !item->parent() || !item->parent()->dir)
        return;

    FileBrowserItem *parentItem = item->parent();
    if (flatMode) {
        goToFlatItem(parentItem);
        return;
    }

    const QString parentPath = tree_model->createRemotePath(parentItem);
    const QModelIndex parentSrc = tree_model->createIndexForItem(parentItem);
    QItemSelectionModel *selection_model = treeView_LPANE->selectionModel();

    disconnect(selection_model, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(slotLeftPaneSelChanged(QItemSelection,QItemSelection)));

    SelPair sparent;
    sparent.index = parentSrc;
    sparent.dir = parentItem->dir;
    sparent.path_tesxt = parentPath;
    changeRoot(parentItem->dir);
    lineEdit_PATH->setText(parentPath);
    updateUpButton();
    applyViewFiltersNow();
    if (parentSrc.isValid()) {
        selection_model->setCurrentIndex(treeMapFromSource(parentSrc),
                QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }

    pathHistory.append(sparent);
    pathHistory_iter = pathHistory.end();

    connect(selection_model, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(slotLeftPaneSelChanged(QItemSelection,QItemSelection)));
}

void ShareBrowser::slotButtonBack(){
    if (flatMode) {
        FileBrowserItem *item = tree_model->createRootForPath(lineEdit_PATH->text());
        if (item)
            goToFlatItem(item->prevSibling());
        return;
    }

    if (!pathHistory.isEmpty()){

        disconnect(treeView_LPANE->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                this, SLOT(slotLeftPaneSelChanged(QItemSelection,QItemSelection)));

        if(pathHistory.end() == pathHistory_iter || pathHistory.begin() != pathHistory_iter)
            --pathHistory_iter;

        SelPair sp= *pathHistory_iter;
        changeRoot(sp.dir);
        lineEdit_PATH->setText(sp.path_tesxt);
        updateUpButton();
        applyViewFiltersNow();

        slotRightPaneClicked(sp.index);

        connect(treeView_LPANE->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                this, SLOT(slotLeftPaneSelChanged(QItemSelection,QItemSelection)));
    }
}

void ShareBrowser::slotButtonForward(){
    if (flatMode) {
        FileBrowserItem *item = tree_model->createRootForPath(lineEdit_PATH->text());
        if (item)
            goToFlatItem(item->nextSibling());
        return;
    }

    if (!pathHistory.isEmpty()){

        disconnect(treeView_LPANE->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                this, SLOT(slotLeftPaneSelChanged(QItemSelection,QItemSelection)));

        if (pathHistory.end() == pathHistory_iter)
            --pathHistory_iter;
        else if (pathHistory_iter != (pathHistory.end() - 1))
            ++pathHistory_iter;

        SelPair sp= *pathHistory_iter;
        changeRoot(sp.dir);
        lineEdit_PATH->setText(sp.path_tesxt);
        updateUpButton();
        applyViewFiltersNow();

        slotRightPaneClicked(sp.index);

        connect(treeView_LPANE->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                this, SLOT(slotLeftPaneSelChanged(QItemSelection,QItemSelection)));
    }
}

void ShareBrowser::slotLayoutUpdated(){
    if (flatMode)
        return;

    QItemSelectionModel *selection_model = treeView_LPANE->selectionModel();
    QModelIndexList selected  = selection_model->selectedRows(0);

    if (selected.size() > 1 || selected.empty())
        return;

    const QModelIndex src = treeMapToSource(selected.at(0));
    if (!src.isValid())
        return;

    const QModelIndex viewIdx = treeMapFromSource(src);
    if (!viewIdx.isValid())
        return;

    selection_model->setCurrentIndex(viewIdx,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void ShareBrowser::slotHeaderMenu(){
    WulforUtil::headerMenu(treeView_RPANE, ShareListColumns::menuSkip());
}

void ShareBrowser::slotSettingsChanged(const QString &key, const QString&){
    if (key == WS_TRANSLATION_FILE)
        retranslateUi(this);
}

void ShareBrowser::slotClose() {
    ArenaWidgetManager::getInstance()->rem(this);
}

void ShareBrowser::slotAddToFavorites() {
    if (user && user != ClientManager::getInstance()->getMe())
        FavoriteManager::getInstance()->addFavoriteUser(user);
}

void ShareBrowser::slotDie(const QString &msg){
    QMessageBox::warning(MainWindow::getInstance(), tr("Share browser"), msg, QMessageBox::Ok);

    slotClose();
}
