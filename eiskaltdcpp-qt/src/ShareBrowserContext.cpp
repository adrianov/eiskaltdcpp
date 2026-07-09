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
#include "DownloadToHistory.h"

#include "dcpp/SettingsManager.h"

#include <QFileDialog>
#include <QDir>

using namespace dcpp;

void ShareBrowser::slotCustomContextMenu(const QPoint &){
    QTreeView *view = dynamic_cast<QTreeView*>(sender());

    if (!view)
        return;

    QItemSelectionModel *selection_model = view->selectionModel();
    QModelIndexList list;
    QModelIndexList selected  = selection_model->selectedRows(0);

    if (view == treeView_RPANE && treeView_RPANE->model() == proxy){
        for (const QModelIndex &i : selected)
            list.push_back(proxy->mapToSource(i));
    }
    else
        list = selected;

    if (!Menu::getInstance())
        Menu::newInstance();

    Menu::Action act = Menu::getInstance()->exec(view == treeView_LPANE? user : dcpp::UserPtr(nullptr));
    QString target = _q(SETTING(DOWNLOAD_DIRECTORY));

    switch (act){
        case Menu::None:
        {
            break;
        }
        case Menu::Download:
        {
            for (const auto &index : list){
                FileBrowserItem *item = reinterpret_cast<FileBrowserItem*>(index.internalPointer());

                if (item->file)
                    download(item->file, target);
                else if (item->dir)
                    download(item->dir, target);
            }

            break;
        }
        case Menu::DownloadTo:
        {
            static QString old_target = QDir::homePath();
            target = Menu::getInstance()->getTarget();

            if (!QDir(target).exists() || target.isEmpty())
                target = QFileDialog::getExistingDirectory(this, tr("Select directory"), old_target);

            if (target.isEmpty())
                break;

            target = QDir::toNativeSeparators(target);

            if (!target.endsWith(QDir::separator()))
                target += QDir::separator();

            old_target = target;

            QStringList temp_pathes = DownloadToDirHistory::get();
            temp_pathes.push_front(target);

            DownloadToDirHistory::put(temp_pathes);

            if (!target.isEmpty()){
                for (const auto &index : list){
                    FileBrowserItem *item = reinterpret_cast<FileBrowserItem*>(index.internalPointer());

                    if (item->file)
                        download(item->file, target);
                    else if (item->dir)
                        download(item->dir, target);
                }
            }

            break;
        }
        case Menu::Alternates:
        case Menu::CopyFileName:
        case Menu::Magnet:
        case Menu::MagnetWeb:
        case Menu::MagnetInfo:
        case Menu::AddToFav:
        case Menu::AddRestrinction:
        case Menu::RemoveRestriction:
        case Menu::OpenUrl:
            contextMoreActions(act, list);
            break;
        default: break;
    }
}
