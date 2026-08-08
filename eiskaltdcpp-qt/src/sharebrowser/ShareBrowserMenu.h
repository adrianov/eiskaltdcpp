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

/** Context menu for ShareBrowser list/tree panes (download, magnets, own-list actions). */
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
        DeleteFile,
        None
    };

    Action exec(const dcpp::UserPtr &user = dcpp::UserPtr(nullptr), bool treePane = false,
                bool hasDeletable = false);
    QString getTarget() { return target; }

private:
    ShareBrowserMenu();
    virtual ~ShareBrowserMenu();

    QMap<QAction *, Action> actions;
    QMenu *menu;
    QMenu *down_to;
    QMenu *down_wh_to;
    QMenu *rest_menu;
    QString target;
    QAction *open_file;
    QAction *open_url;
    QAction *delete_file;
};
