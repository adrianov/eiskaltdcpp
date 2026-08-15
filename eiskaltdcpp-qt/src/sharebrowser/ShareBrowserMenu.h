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
#include <QMap>
#include <QMenu>
#include <QString>

#include "dcpp/stdinc.h"
#include "dcpp/User.h"
#include "dcpp/Singleton.h"

class DownloadToMenu;

/** Context menu for ShareBrowser panes: download, magnets, open, EPUB export, rename, delete. */
class ShareBrowserMenu : public dcpp::Singleton<ShareBrowserMenu> {
    friend class dcpp::Singleton<ShareBrowserMenu>;

public:
    enum Action {
        Download = 0,
        DownloadTo,
        DownloadWholeDir,
        DownloadWholeDirTo,
        Alternates,
        CopyFileName,
        Magnet,
        MagnetWeb,
        MagnetInfo,
        AddToFav,
        AddRestrinction,
        RemoveRestriction,
        OpenFile,
        OpenUrl,
        ConvertEpub,
        DeleteFile,
        DeleteWholeDir,
        RenameFolder,
        None
    };

    struct Flags {
        bool treePane = false;
        bool deletable = false;
        bool fb2 = false;
        bool renameFolder = false;
        bool deleteWholeDir = false;
    };

    Action exec(const dcpp::UserPtr &user, const Flags &flags);
    QString getTarget() { return target; }

private:
    ShareBrowserMenu();
    virtual ~ShareBrowserMenu();

    QMap<QAction *, Action> actions;
    QMenu *menu;
    DownloadToMenu *down_to;
    DownloadToMenu *down_wh_to;
    QMenu *rest_menu;
    QString target;
    QAction *open_file;
    QAction *open_url;
    QAction *convert_epub;
    QAction *delete_file;
    QAction *delete_whole_dir;
    QAction *rename_folder;
};
