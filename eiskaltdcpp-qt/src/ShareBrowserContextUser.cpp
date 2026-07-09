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

#include "dcpp/FavoriteManager.h"
#include "dcpp/ClientManager.h"
#include "dcpp/ShareManager.h"

#include <QInputDialog>
#include <QDesktopServices>
#include <QDir>
#include <QUrl>

using namespace dcpp;

void ShareBrowser::contextUserActions(Menu::Action act, const QModelIndexList &list)
{
    switch (act){
        case Menu::AddToFav:
        {
            if (user && user != ClientManager::getInstance()->getMe())
                FavoriteManager::getInstance()->addFavoriteUser(user);

            break;
        }
        case Menu::AddRestrinction:
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
        case Menu::RemoveRestriction:
        {
            for (const QModelIndex &index : list) {
                QModelIndex idx = index;
                tree_model->updateRestriction(idx, 0);
            }

            break;
        }
        case Menu::OpenUrl:
        {
            ShareManager *SM = ShareManager::getInstance();

            for (const auto &index : list){
                FileBrowserItem *item = reinterpret_cast<FileBrowserItem*>(index.internalPointer());

                if (!item)
                    continue;

                DirectoryListing::AdlDirectory *adl_dir = dynamic_cast<DirectoryListing::AdlDirectory*>(item->dir);
                dcpp::StringList lst;

                try {
                    if (!adl_dir)
                        lst  = listing.getLocalPaths(item->dir);
                    else{
                        QStringList path_lst = _q(adl_dir->getFullPath()).split('\\');

                        if (path_lst.size() < 1)
                            break;

                        path_lst.removeFirst();//remove root element

                        QString path = path_lst.join("\\")+'\\';

                        lst = SM->getRealPaths(Util::toAdcFile(_tq(path)));
                    }
                }
                catch ( ... ){ }

                for (const auto &it : lst){
                    if (QDir(_q(it)).exists())
#ifndef Q_OS_WIN
                        QDesktopServices::openUrl(QUrl("file://"+_q(it)));
#else
                        QDesktopServices::openUrl(QUrl("file:///"+_q(it)));
#endif
                }
            }

            break;
        }
        default: break;
    }
}
