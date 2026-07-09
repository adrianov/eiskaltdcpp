/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SearchFrame.h"
#include "SearchFramePrivate.h"
#include "SearchModel.h"
#include "SearchBlacklist.h"
#include "SearchBlacklistDialog.h"
#include "WulforUtil.h"
#include "Magnet.h"
#include "ArenaWidgetFactory.h"
#include "DownloadToHistory.h"
#include "HubFrame.h"
#include "HubManager.h"
#include "MainWindow.h"

#include "dcpp/ClientManager.h"
#include "dcpp/FavoriteManager.h"
#include "dcpp/UserCommand.h"
#include "dcpp/SettingsManager.h"

#include <QClipboard>
#include <QFileDialog>
#include <QDir>

using namespace dcpp;

void SearchFrame::slotContextMenu(const QPoint &){
    QItemSelectionModel *selection_model = treeView_RESULTS->selectionModel();
    QModelIndexList list = selection_model->selectedRows(0);
    Q_D(SearchFrame);

    if (list.size() < 1)
        return;

    if (d->proxy)
        std::transform(list.begin(), list.end(), list.begin(), [&d](QModelIndex i) { return d->proxy->mapToSource(i); } );

    if (!Menu::getInstance())
        Menu::newInstance();

    QStringList hubs;

    for (const auto &i : list){
        SearchItem *item = reinterpret_cast<SearchItem*>(i.internalPointer());
        QString host = item->data(COLUMN_SF_HOST).toString();

        if (!hubs.contains(host))
            hubs.push_back(host);
    }

    Menu::Action act = Menu::getInstance()->exec(hubs);

    switch (act){
        case Menu::None:
        {
            break;
        }
        case Menu::Download:
        case Menu::DownloadTo:
        case Menu::DownloadWholeDir:
        case Menu::DownloadWholeDirTo:
            contextDownloads(act, list);
            break;
        case Menu::SearchTTH:
        case Menu::Magnet:
        case Menu::MagnetWeb:
        case Menu::MagnetInfo:
        case Menu::Browse:
        case Menu::MatchQueue:
            contextMoreActions(act, list);
            break;
        case Menu::SendPM:
        case Menu::AddToFav:
        case Menu::GrantExtraSlot:
        case Menu::RemoveFromQueue:
        case Menu::Remove:
        case Menu::UserCommands:
        case Menu::Blacklist:
        case Menu::AddToBlacklist:
            contextUserActions(act, list);
            break;
        default:
        {
            break;
        }
    }
}

