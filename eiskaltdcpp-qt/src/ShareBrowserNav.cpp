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
#include "SearchLocalPath.h"

#include "dcpp/ClientManager.h"
#include "dcpp/SettingsManager.h"

#include <QAbstractItemView>

using namespace dcpp;

void ShareBrowser::goDown(QTreeView *view){
    if (view != treeView_RPANE)
        return;

    QItemSelectionModel *selection_model = view->selectionModel();
    QModelIndexList selected  = selection_model->selectedRows(0);

    if (selected.size() > 1 || selected.empty())
        return;

    const QModelIndex index = selected.at(0);
    FileBrowserItem *item = nullptr;

    if (view->model() == proxy)
        item = static_cast<FileBrowserItem*>(proxy->mapToSource(index).internalPointer());
    else
        item = static_cast<FileBrowserItem*>(index.internalPointer());

    if (!item || item->file)
        return;

    slotRightPaneClicked(index);

    treeView_RPANE->setFocus();
}

void ShareBrowser::goUp(QTreeView *view){
    if (view != treeView_RPANE)
        return;

    QStringList paths = lineEdit_PATH->text().split("\\", WULFOR_SKIP_EMPTY);

    if (paths.empty())//is it possible?
        return;
    else
        paths.removeLast();

    FileBrowserItem *tree_item = tree_model->createRootForPath(paths.join("\\"));
    QModelIndex tree_index = tree_model->createIndexForItem(tree_item);

    treeView_LPANE->selectionModel()->setCurrentIndex(tree_index,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    treeView_LPANE->scrollTo(tree_index, QAbstractItemView::PositionAtCenter);

    treeView_RPANE->setFocus();
}

void ShareBrowser::download(dcpp::DirectoryListing::Directory *dir, const QString &target){
    if (dir && !target.isEmpty()){
        try {
            listing.download(dir, target.toStdString(), false);
        }
        catch (const Exception &e){
            MainWindow::getInstance()->setStatusMessage(_q(e.getError()));
        }
    }
}

void ShareBrowser::download(dcpp::DirectoryListing::File *file, const QString &target){
    if (file && !target.isEmpty()){
        QString name = _q(file->getName());

        try {
            listing.download(file, (target+name).toStdString(), false, false);
        }
        catch (const Exception &e){
            MainWindow::getInstance()->setStatusMessage(_q(e.getError()));
        }
    }
}

void ShareBrowser::slotRightPaneClicked(const QModelIndex &index){
    if (!index.isValid())
        return;

    FileBrowserItem *item = nullptr;

    if (treeView_RPANE->model() == proxy)
        item = static_cast<FileBrowserItem*>(proxy->mapToSource(index).internalPointer());
    else
        item = static_cast<FileBrowserItem*>(index.internalPointer());

    if (!item)
        return;

    if (item->file){
        if (user == ClientManager::getInstance()->getMe()) {
            for (const auto &path : listing.getLocalPaths(item->file))
                SearchLocalPath::openFile(_q(path));
        } else {
            download(item->file, _q(SETTING(DOWNLOAD_DIRECTORY)));
        }

        return;
    }

    QString parent_path = lineEdit_PATH->text() + "\\" + item->data(COLUMN_FILEBROWSER_NAME).toString();
    QModelIndex parent_index = tree_model->createIndexForItem(tree_model->createRootForPath(parent_path));

    if (!parent_index.isValid())
        return;

    if (tree_model->canFetchMore(parent_index))
        tree_model->fetchMore(parent_index);

    treeView_LPANE->selectionModel()->setCurrentIndex(parent_index,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    treeView_LPANE->scrollTo(parent_index, QAbstractItemView::PositionAtCenter);
}

