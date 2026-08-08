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
#include "search/SearchLocalPath.h"

#include "dcpp/FavoriteManager.h"
#include "dcpp/ClientManager.h"
#include "dcpp/ShareManager.h"

#include <QInputDialog>

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

} // namespace

void ShareBrowser::contextUserActions(ShareBrowserMenu::Action act, const QModelIndexList &list)
{
    switch (act){
        case ShareBrowserMenu::AddToFav:
        {
            if (user && user != ClientManager::getInstance()->getMe())
                FavoriteManager::getInstance()->addFavoriteUser(user);

            break;
        }
        case ShareBrowserMenu::AddRestrinction:
        {
            bool ok = false;
            unsigned share_sz = QInputDialog::getInt(this, tr("Enter restriction size (in GB)"), "Size", 0, 0, 1024, 1, &ok);

            if (!ok)
                break;

            for (const QModelIndex &index : list) {
                QModelIndex idx = index;
                tree_model->updateRestriction(idx, share_sz);
            }

            break;
        }
        case ShareBrowserMenu::RemoveRestriction:
        {
            for (const QModelIndex &index : list) {
                QModelIndex idx = index;
                tree_model->updateRestriction(idx, 0);
            }

            break;
        }
        case ShareBrowserMenu::OpenFile:
        case ShareBrowserMenu::OpenUrl:
        {
            const bool reveal = (act == ShareBrowserMenu::OpenUrl);
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
            break;
        }
        case ShareBrowserMenu::DeleteFile:
            deleteOwnItems(list);
            break;
        default: break;
    }
}
