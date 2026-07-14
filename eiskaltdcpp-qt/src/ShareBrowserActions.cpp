/***************************************************************************
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

#include "dcpp/FavoriteManager.h"
#include "dcpp/ClientManager.h"

#include <QMessageBox>

using namespace dcpp;

void ShareBrowser::slotButtonUp(){

    QItemSelectionModel *selection_model = treeView_LPANE->selectionModel();
    QModelIndexList selected  = selection_model->selectedRows(0);

    if (selected.size() > 1 || selected.empty())
        return;

    QModelIndex index = selected.at(0);
    index = treeMapToSource(index);

    if (index.isValid()){

        FileBrowserItem *item = static_cast<FileBrowserItem*>(index.internalPointer());

        if (nullptr != item->parent()){

            disconnect(treeView_LPANE->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                    this, SLOT(slotLeftPaneSelChanged(QItemSelection,QItemSelection)));

            SelPair sparent;
            sparent.index = index.parent();

            changeRoot(item->parent()->dir);
            sparent.dir = item->parent()->dir;

            sparent.path_tesxt = tree_model->createRemotePath(item->parent());
            lineEdit_PATH->setText(sparent.path_tesxt);
            applyViewFiltersNow();

            slotRightPaneClicked(index.parent());

            pathHistory.append(sparent);
            pathHistory_iter = pathHistory.end();

            connect(treeView_LPANE->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                    this, SLOT(slotLeftPaneSelChanged(QItemSelection,QItemSelection)));
        }
    }
}

void ShareBrowser::slotButtonBack(){
    if (pathHistory_iter && !pathHistory.isEmpty()){

        disconnect(treeView_LPANE->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                this, SLOT(slotLeftPaneSelChanged(QItemSelection,QItemSelection)));

        if(pathHistory.end() == pathHistory_iter || pathHistory.begin() != pathHistory_iter)
            --pathHistory_iter;

        SelPair sp= *pathHistory_iter;
        changeRoot(sp.dir);
        lineEdit_PATH->setText(sp.path_tesxt);
        applyViewFiltersNow();

        slotRightPaneClicked(sp.index);

        connect(treeView_LPANE->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                this, SLOT(slotLeftPaneSelChanged(QItemSelection,QItemSelection)));
    }
}

void ShareBrowser::slotButtonForward(){
    if (pathHistory_iter && !pathHistory.isEmpty()){

        disconnect(treeView_LPANE->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                this, SLOT(slotLeftPaneSelChanged(QItemSelection,QItemSelection)));

        if (pathHistory.end() == pathHistory_iter)
            --pathHistory_iter;
        else if (pathHistory_iter != &pathHistory.last())
            ++pathHistory_iter;

        SelPair sp= *pathHistory_iter;
        changeRoot(sp.dir);
        lineEdit_PATH->setText(sp.path_tesxt);
        applyViewFiltersNow();

        slotRightPaneClicked(sp.index);

        connect(treeView_LPANE->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                this, SLOT(slotLeftPaneSelChanged(QItemSelection,QItemSelection)));
    }
}

void ShareBrowser::slotLayoutUpdated(){
    QItemSelectionModel *selection_model = treeView_LPANE->selectionModel();
    QModelIndexList selected  = selection_model->selectedRows(0);

    if (selected.size() > 1 || selected.empty())
        return;

    QModelIndex index = selected.at(0);
    index = treeMapToSource(index);

    treeView_LPANE->selectionModel()->setCurrentIndex(treeMapFromSource(index),
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void ShareBrowser::slotHeaderMenu(){
    WulforUtil::headerMenu(treeView_RPANE);
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
