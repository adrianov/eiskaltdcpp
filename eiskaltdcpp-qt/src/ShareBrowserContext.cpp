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
#include "DownloadToHistory.h"

#include "dcpp/SettingsManager.h"

#include <QFileDialog>
#include <QDir>
#include <QSet>

using namespace dcpp;

namespace {

DirectoryListing::Directory *wholeDir(FileBrowserItem *item)
{
    if (!item)
        return nullptr;
    if (item->dir)
        return item->dir;
    if (item->file)
        return item->file->getParent();
    return nullptr;
}

QString pickDownloadTarget(QWidget *parent, const QString &title, const QString &chosen, QString &oldTarget)
{
    QString target = chosen;

    if (!QDir(target).exists() || target.isEmpty())
        target = QFileDialog::getExistingDirectory(parent, title, oldTarget);

    if (target.isEmpty())
        return QString();

    target = QDir::toNativeSeparators(target);

    if (!target.endsWith(QDir::separator()))
        target += QDir::separator();

    oldTarget = target;

    QStringList temp_pathes = DownloadToDirHistory::get();
    temp_pathes.push_front(target);
    DownloadToDirHistory::put(temp_pathes);

    return target;
}

} // namespace

void ShareBrowser::slotCustomContextMenu(const QPoint &){
    QTreeView *view = dynamic_cast<QTreeView*>(sender());

    if (!view)
        return;

    QItemSelectionModel *selection_model = view->selectionModel();
    QModelIndexList list;
    QModelIndexList selected  = selection_model->selectedRows(0);

    if (view == treeView_RPANE && proxy){
        for (const QModelIndex &i : selected)
            list.push_back(proxy->mapToSource(i));
    }
    else if (view == treeView_LPANE && tree_proxy){
        for (const QModelIndex &i : selected)
            list.push_back(tree_proxy->mapToSource(i));
    }
    else
        list = selected;

    if (!ShareBrowserMenu::getInstance())
        ShareBrowserMenu::newInstance();

    bool hasDeletable = false;
    for (const auto &index : list) {
        FileBrowserItem *item = reinterpret_cast<FileBrowserItem*>(index.internalPointer());
        if (!item)
            continue;
        if (item->file) {
            hasDeletable = true;
            break;
        }
        if (item->dir && item->dir != listing.getRoot()
                && !dynamic_cast<dcpp::DirectoryListing::AdlDirectory*>(item->dir)) {
            hasDeletable = true;
            break;
        }
    }

    ShareBrowserMenu::Action act = ShareBrowserMenu::getInstance()->exec(user, view == treeView_LPANE, hasDeletable);
    QString target = _q(SETTING(DOWNLOAD_DIRECTORY));

    switch (act){
        case ShareBrowserMenu::None:
        {
            break;
        }
        case ShareBrowserMenu::Download:
        case ShareBrowserMenu::DownloadTo:
        {
            static QString old_target = QDir::homePath();
            if (act == ShareBrowserMenu::DownloadTo) {
                target = pickDownloadTarget(this, tr("Select directory"),
                                            ShareBrowserMenu::getInstance()->getTarget(), old_target);
                if (target.isEmpty())
                    break;
            }

            for (const auto &index : list){
                FileBrowserItem *item = reinterpret_cast<FileBrowserItem*>(index.internalPointer());
                if (!item)
                    continue;
                if (item->file)
                    download(item->file, target);
                else if (item->dir)
                    download(item->dir, target);
            }

            break;
        }
        case ShareBrowserMenu::DownloadWholeDir:
        case ShareBrowserMenu::DownloadWholeDirTo:
        {
            static QString old_target = QDir::homePath();
            if (act == ShareBrowserMenu::DownloadWholeDirTo) {
                target = pickDownloadTarget(this, tr("Select directory"),
                                            ShareBrowserMenu::getInstance()->getTarget(), old_target);
                if (target.isEmpty())
                    break;
            }

            QSet<DirectoryListing::Directory*> dirs;
            for (const auto &index : list){
                FileBrowserItem *item = reinterpret_cast<FileBrowserItem*>(index.internalPointer());
                if (DirectoryListing::Directory *dir = wholeDir(item))
                    dirs.insert(dir);
            }

            for (DirectoryListing::Directory *dir : dirs)
                download(dir, target);

            break;
        }
        case ShareBrowserMenu::Alternates:
        case ShareBrowserMenu::CopyFileName:
        case ShareBrowserMenu::Magnet:
        case ShareBrowserMenu::MagnetWeb:
        case ShareBrowserMenu::MagnetInfo:
        case ShareBrowserMenu::AddToFav:
        case ShareBrowserMenu::AddRestrinction:
        case ShareBrowserMenu::RemoveRestriction:
        case ShareBrowserMenu::OpenFile:
        case ShareBrowserMenu::OpenUrl:
        case ShareBrowserMenu::DeleteFile:
            contextMoreActions(act, list);
            break;
        default: break;
    }
}
