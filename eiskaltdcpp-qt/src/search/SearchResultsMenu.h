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

#pragma once

#include <QAction>
#include <QList>
#include <QMap>
#include <QMenu>
#include <QString>
#include <QStringList>

#include "dcpp/stdinc.h"
#include "dcpp/Singleton.h"

class DownloadToMenu;

/** Context menu for Search results: download, magnets, user actions, blacklist. */
class SearchResultsMenu : public dcpp::Singleton<SearchResultsMenu> {
    friend class dcpp::Singleton<SearchResultsMenu>;

public:
    enum Action {
        Download = 0,
        DownloadTo,
        DownloadWholeDir,
        DownloadWholeDirTo,
        OpenFile,
        OpenDirectory,
        SearchTTH,
        CopyFileName,
        Magnet,
        MagnetWeb,
        MagnetInfo,
        Browse,
        MatchQueue,
        SendPM,
        AddToFav,
        GrantExtraSlot,
        RemoveFromQueue,
        Remove,
        UserCommands,
        Blacklist,
        AddToBlacklist,
        None
    };

    Action exec(const QStringList &, bool canOpenLocal = false);
    QMenu *buildUserCmdMenu(QList<QString> hubs);
    QString getDownloadToPath() { return downToPath; }
    int getCommandId() { return uc_cmd_id; }
    void addTempPath(const QString &path);

private:
    SearchResultsMenu();
    virtual ~SearchResultsMenu();

    QMap<QAction *, Action> actions;
    QList<QAction *> action_list;

    QString downToPath;
    int uc_cmd_id = 0;

    QMenu *menu = nullptr;
    QMenu *magnet_menu = nullptr;
    DownloadToMenu *down_to = nullptr;
    DownloadToMenu *down_wh_to = nullptr;
    QMenu *black_list_menu = nullptr;
};
