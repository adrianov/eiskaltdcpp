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
#include "fb2epub/Fb2EpubExport.h"
#include "search/SearchLocalPath.h"

#include "dcpp/FavoriteManager.h"
#include "dcpp/ClientManager.h"
#include "dcpp/ShareManager.h"

#include <QInputDialog>
#include <QStringList>

using namespace dcpp;

namespace {

StringList localPaths(DirectoryListing &listing, FileBrowserItem *item)
{
    if (!item)
        return StringList();

    try {
        if (item->file)
            return listing.getLocalPaths(item->file);

        if (!item->dir)
            return StringList();

        auto *adl = dynamic_cast<DirectoryListing::AdlDirectory*>(item->dir);
        if (!adl)
            return listing.getLocalPaths(item->dir);

        QStringList parts = _q(adl->getFullPath()).split('\\');
        if (parts.isEmpty())
            return StringList();
        parts.removeFirst();
        return ShareManager::getInstance()->getRealPaths(
            Util::toAdcFile(_tq(parts.join("\\") + '\\')));
    } catch (...) {
        return StringList();
    }
}

void openLocals(DirectoryListing &listing, const QModelIndexList &list, bool reveal)
{
    for (const auto &index : list) {
        FileBrowserItem *item = reinterpret_cast<FileBrowserItem*>(index.internalPointer());
        if (!item)
            continue;
        for (const auto &path : localPaths(listing, item)) {
            const QString local = _q(path);
            if (reveal)
                SearchLocalPath::openDirectory(local);
            else
                SearchLocalPath::openFile(local);
        }
    }
}

void convertFb2(DirectoryListing &listing, const QModelIndexList &list)
{
    QStringList paths;
    for (const auto &index : list) {
        FileBrowserItem *item = reinterpret_cast<FileBrowserItem*>(index.internalPointer());
        if (!item || !item->file)
            continue;
        for (const auto &path : localPaths(listing, item))
            paths.push_back(_q(path));
    }
    Fb2EpubExport::convertAndReveal(paths);
}

void setShareLimit(FileBrowserModel *model, const QModelIndexList &list, unsigned size)
{
    for (const QModelIndex &index : list) {
        QModelIndex idx = index;
        model->updateRestriction(idx, size);
    }
}

} // namespace

void ShareBrowser::contextUserActions(ShareBrowserMenu::Action act, const QModelIndexList &list)
{
    switch (act) {
        case ShareBrowserMenu::AddToFav:
            if (user && user != ClientManager::getInstance()->getMe())
                FavoriteManager::getInstance()->addFavoriteUser(user);
            break;
        case ShareBrowserMenu::AddRestrinction: {
            bool ok = false;
            unsigned share_sz = QInputDialog::getInt(this, tr("Enter restriction size (in GB)"),
                                                     "Size", 0, 0, 1024, 1, &ok);
            if (ok)
                setShareLimit(tree_model, list, share_sz);
            break;
        }
        case ShareBrowserMenu::RemoveRestriction:
            setShareLimit(tree_model, list, 0);
            break;
        case ShareBrowserMenu::OpenFile:
            openLocals(listing, list, false);
            break;
        case ShareBrowserMenu::OpenUrl:
            openLocals(listing, list, true);
            break;
        case ShareBrowserMenu::ConvertEpub:
            convertFb2(listing, list);
            break;
        case ShareBrowserMenu::DeleteFile:
            deleteOwnItems(list);
            break;
        case ShareBrowserMenu::DeleteOtherCopies:
            deleteOwnOtherCopies(list);
            break;
        case ShareBrowserMenu::DeleteWholeDir:
            deleteOwnWholeDir(list);
            break;
        case ShareBrowserMenu::RenameFolder:
            renameOwnFolder(list);
            break;
        default:
            break;
    }
}
