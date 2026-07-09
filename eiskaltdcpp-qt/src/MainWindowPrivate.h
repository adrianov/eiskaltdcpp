/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QAction>
#include <QActionGroup>
#include <QDockWidget>
#include <QHash>
#include <QLabel>
#include <QList>
#include <QMenu>
#include <QProgressBar>

#include "ArenaWidget.h"
#include "ToolBar.h"
#include "LineEdit.h"
#include "StatusBarLogLabel.h"
#include "HashProgress.h"

#ifdef USE_JS
class ScriptConsole;
#endif

class MainWindowPrivate {
public:
    typedef QList<QAction*> ActionList;

    bool isUnload = false;
    bool exitBegin = false;

    bool showMax = false;
    int w = 800;
    int h = 600;
    int xPos = 0;
    int yPos = 0;

    QDockWidget *arena = nullptr;
    QDockWidget *transfer_dock = nullptr;
    QDockWidget *sideDock = nullptr;

    ToolBar *fBar = nullptr;
    ToolBar *sBar = nullptr;

    LineEdit *searchLineEdit = nullptr;
    QLabel *statusLabel = nullptr;
    QLabel *statusSPLabel = nullptr;
    QLabel *statusDLabel = nullptr;
    QLabel *statusTRLabel = nullptr;
    StatusBarLogLabel *msgLabel = nullptr;

#if defined(USE_PROGRESS_BARS)
    QProgressBar *progressFreeSpace = nullptr;
#else
    QLabel *progressFreeSpace = nullptr;
#endif
    QProgressBar *progressHashing = nullptr;
    HashProgress *_progress_dialog = nullptr;

    QMenu   *menuFile = nullptr;
    QAction *fileOpenMagnet = nullptr;
    QAction *fileFileListBrowser = nullptr;
    QAction *fileFileHasher = nullptr;
    QAction *fileFileListBrowserLocal = nullptr;
    QAction *fileFileListMatchAll = nullptr;
    QAction *fileRefreshShareHashProgress = nullptr;
    QAction *fileOpenLogFile = nullptr;
    QAction *fileOpenDownloadDirectory = nullptr;
    QAction *fileHideWindow = nullptr;
    QAction *fileQuit = nullptr;

    QMenu   *menuHubs = nullptr;
    QAction *hubsHubReconnect = nullptr;
    QAction *hubsQuickConnect = nullptr;
    QAction *hubsFavoriteHubs = nullptr;
    QAction *hubsPublicHubs = nullptr;
    QAction *hubsFavoriteUsers = nullptr;

    QMenu   *menuTools = nullptr;
    QAction *toolsSearch = nullptr;
    QAction *toolsADLS = nullptr;
    QAction *toolsCmdDebug = nullptr;
    QAction *toolsTransfers = nullptr;
    QAction *toolsDownloadQueue = nullptr;
    QAction *toolsQueuedUsers = nullptr;
    QAction *toolsFinishedDownloads = nullptr;
    QAction *toolsFinishedUploads = nullptr;
    QAction *toolsSecretary = nullptr;
    QAction *toolsSearchSpy = nullptr;
    QAction *toolsAntiSpam = nullptr;
    QAction *toolsIPFilter = nullptr;
    QAction *menuAwayAction = nullptr;
    QAction *toolsHubManager = nullptr;

    QMenu   *menuAway = nullptr;
    QActionGroup *awayGroup = nullptr;
    QAction *toolsAwayOn = nullptr;
    QAction *toolsAwayOff = nullptr;
    QAction *toolsAutoAway = nullptr;

    QAction *toolsHideProgressSpace = nullptr;
    QAction *toolsHideLastStatus = nullptr;
    QAction *toolsHideUsersStatisctics = nullptr;
    QAction *toolsCopyWindowTitle = nullptr;
    QAction *toolsOptions = nullptr;
#ifdef USE_JS
    QAction *toolsJS = nullptr;
    QAction *toolsJSConsole = nullptr;
    ScriptConsole *scriptConsole = nullptr;
#endif
    QAction *toolsSwitchSpeedLimit = nullptr;

    QMenu   *menuPanels = nullptr;
    QMenu   *sh_menu = nullptr;
    QAction *panelsWidgets = nullptr;
    QAction *panelsTools = nullptr;
    QAction *panelsSearch = nullptr;

    QAction *prevTabShortCut = nullptr;
    QAction *nextTabShortCut = nullptr;
    QAction *prevMsgShortCut = nullptr;
    QAction *nextMsgShortCut = nullptr;
    QAction *closeWidgetShortCut = nullptr;
    QAction *toggleMainMenuShortCut = nullptr;

    QAction *chatDisable = nullptr;
    QAction *findInWidget = nullptr;
    QAction *chatClear = nullptr;

    QMenu *menuWidgets = nullptr;
    QHash<QAction*, ArenaWidget*> menuWidgetsHash;

    QMenu   *menuAbout = nullptr;
    QAction *aboutHomepage = nullptr;
    QAction *aboutBuilds = nullptr;
    QAction *aboutSource = nullptr;
    QAction *aboutIssues = nullptr;
    QAction *aboutWiki = nullptr;
    QAction *aboutChangelog = nullptr;
    QAction *aboutClient = nullptr;
    QAction *aboutQt = nullptr;

    ActionList toolBarActions;
    ActionList fileMenuActions;
    ActionList hubsMenuActions;
    ActionList toolsMenuActions;

    QMenu *favHubMenu = nullptr;
};
