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

#include "mainwindow/actions/ActionCatalog.h"
#include "MainWindow.h"
#include "MainWindowPrivate.h"
#include "Antispam.h"
#include "WulforUtil.h"
#include "WulforSettings.h"
#include "ShortcutManager.h"
#include "appicon/AppIcons.h"

#include "dcpp/SettingsManager.h"
#include "dcpp/Util.h"

#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QObject>

using namespace dcpp;

void ActionCatalog::build()
{
    WulforUtil *WU = WulforUtil::getInstance();
    ShortcutManager *SM = ShortcutManager::getInstance();

    d->fileOpenMagnet = new QAction("", host);
    d->fileOpenMagnet->setObjectName("fileOpenMagnet");
    SM->registerShortcut(d->fileOpenMagnet, QString("Ctrl+I"));
    WulforUtil::bindActionIcon(d->fileOpenMagnet, AppIcons::eiDOWNLOAD);
    QObject::connect(d->fileOpenMagnet, SIGNAL(triggered()), host, SLOT(slotOpenMagnet()));

    d->fileFileListBrowserLocal = new QAction("", host);
    d->fileFileListBrowserLocal->setObjectName("fileFileListBrowserLocal");
    SM->registerShortcut(d->fileFileListBrowserLocal, QString("Ctrl+L"));
    WulforUtil::bindActionIcon(d->fileFileListBrowserLocal, AppIcons::eiOWN_FILELIST);
    QObject::connect(d->fileFileListBrowserLocal, SIGNAL(triggered()), host, SLOT(slotFileBrowseOwnFilelist()));

    d->fileFileListBrowser = new QAction("", host);
    d->fileFileListBrowser->setObjectName("fileFileListBrowser");
    WulforUtil::bindActionIcon(d->fileFileListBrowser, AppIcons::eiOPENLIST);
    QObject::connect(d->fileFileListBrowser, SIGNAL(triggered()), host, SLOT(slotFileBrowseFilelist()));

    d->fileFileListMatchAll = new QAction("", host);
    d->fileFileListMatchAll->setObjectName("fileFileListMatchAll");
    QObject::connect(d->fileFileListMatchAll, SIGNAL(triggered()), host, SLOT(slotFileMatchAllList()));

    d->fileFileHasher = new QAction("", host);
    d->fileFileHasher->setObjectName("fileFileHasher");
    WulforUtil::bindActionIcon(d->fileFileHasher, AppIcons::eiOPENLIST);
    QObject::connect(d->fileFileHasher, SIGNAL(triggered()), host, SLOT(slotFileHasher()));

    d->fileOpenLogFile = new QAction("", host);
    d->fileOpenLogFile->setObjectName("fileOpenLogFile");
    WulforUtil::bindActionIcon(d->fileOpenLogFile, AppIcons::eiOPEN_LOG_FILE);
    QObject::connect(d->fileOpenLogFile, SIGNAL(triggered()), host, SLOT(slotFileOpenLogFile()));

    d->fileOpenDownloadDirectory = new QAction("", host);
    d->fileOpenDownloadDirectory->setObjectName("fileOpenDownloadDirectory");
    WulforUtil::bindActionIcon(d->fileOpenDownloadDirectory, AppIcons::eiFOLDER_BLUE);
    QObject::connect(d->fileOpenDownloadDirectory, SIGNAL(triggered()), host, SLOT(slotFileOpenDownloadDirectory()));

    d->fileRefreshShareHashProgress = new QAction("", host);
    d->fileRefreshShareHashProgress->setObjectName("fileRefreshShareHashProgress");
    SM->registerShortcut(d->fileRefreshShareHashProgress, QString("Ctrl+E"));
    WulforUtil::bindActionIcon(d->fileRefreshShareHashProgress, AppIcons::eiHASHING);
    QObject::connect(d->fileRefreshShareHashProgress, SIGNAL(triggered()), host, SLOT(slotFileRefreshShareHashProgress()));

    d->fileHideWindow = new QAction("", host);
    d->fileHideWindow->setObjectName("fileHideWindow");
    SM->registerShortcut(d->fileHideWindow, QString("Ctrl+Alt+H"));
    WulforUtil::bindActionIcon(d->fileHideWindow, AppIcons::eiHIDEWINDOW);
    QObject::connect(d->fileHideWindow, SIGNAL(triggered()), host, SLOT(slotHideWindow()));

    d->fileQuit = new QAction("", host);
    d->fileQuit->setObjectName("fileQuit");
    SM->registerShortcut(d->fileQuit, QString("Ctrl+Q"));
    d->fileQuit->setMenuRole(QAction::QuitRole);
    WulforUtil::bindActionIcon(d->fileQuit, AppIcons::eiEXIT);
    QObject::connect(d->fileQuit, SIGNAL(triggered()), host, SLOT(slotExit()));

    d->hubsHubReconnect = new QAction("", host);
    d->hubsHubReconnect->setObjectName("hubsHubReconnect");
    SM->registerShortcut(d->hubsHubReconnect, QString("Ctrl+R"));
    WulforUtil::bindActionIcon(d->hubsHubReconnect, AppIcons::eiRECONNECT);
    QObject::connect(d->hubsHubReconnect, SIGNAL(triggered()), host, SLOT(slotHubsReconnect()));

    d->hubsQuickConnect = new QAction("", host);
    d->hubsQuickConnect->setObjectName("hubsQuickConnect");
    SM->registerShortcut(d->hubsQuickConnect, QString("Ctrl+N"));
    WulforUtil::bindActionIcon(d->hubsQuickConnect, AppIcons::eiCONNECT);
    QObject::connect(d->hubsQuickConnect, SIGNAL(triggered()), host, SLOT(slotQC()));

    d->hubsFavoriteHubs = new QAction("", host);
    d->hubsFavoriteHubs->setObjectName("hubsFavoriteHubs");
    SM->registerShortcut(d->hubsFavoriteHubs, QString("Ctrl+H"));
    WulforUtil::bindActionIcon(d->hubsFavoriteHubs, AppIcons::eiFAVSERVER);
    QObject::connect(d->hubsFavoriteHubs, SIGNAL(triggered()), host, SLOT(slotHubsFavoriteHubs()));

    d->hubsPublicHubs = new QAction("", host);
    d->hubsPublicHubs->setObjectName("hubsPublicHubs");
    SM->registerShortcut(d->hubsPublicHubs, QString("Ctrl+P"));
    WulforUtil::bindActionIcon(d->hubsPublicHubs, AppIcons::eiSERVER);
    QObject::connect(d->hubsPublicHubs, SIGNAL(triggered()), host, SLOT(slotHubsPublicHubs()));

    d->hubsFavoriteUsers = new QAction("", host);
    d->hubsFavoriteUsers->setObjectName("hubsFavoriteUsers");
    SM->registerShortcut(d->hubsFavoriteUsers, QString("Ctrl+U"));
    WulforUtil::bindActionIcon(d->hubsFavoriteUsers, AppIcons::eiFAVUSERS);
    QObject::connect(d->hubsFavoriteUsers, SIGNAL(triggered()), host, SLOT(slotHubsFavoriteUsers()));

    d->toolsHubManager = new QAction("", host);
    d->toolsHubManager->setObjectName("toolsHubManager");
    WulforUtil::bindActionIcon(d->toolsHubManager, AppIcons::eiSERVER);
    QObject::connect(d->toolsHubManager, SIGNAL(triggered()), host, SLOT(slotToolsHubManager()));

    d->toolsCopyWindowTitle = new QAction("", host);
    d->toolsCopyWindowTitle->setObjectName("toolsCopyWindowTitle");
    WulforUtil::bindActionIcon(d->toolsCopyWindowTitle, AppIcons::eiEDITCOPY);
    QObject::connect(d->toolsCopyWindowTitle, SIGNAL(triggered()), host, SLOT(slotToolsCopyWindowTitle()));

    d->toolsOptions = new QAction("", host);
    d->toolsOptions->setObjectName("toolsOptions");
    SM->registerShortcut(d->toolsOptions, QString("Ctrl+O"));
    d->toolsOptions->setMenuRole(QAction::PreferencesRole);
    WulforUtil::bindActionIcon(d->toolsOptions, AppIcons::eiCONFIGURE);
    QObject::connect(d->toolsOptions, SIGNAL(triggered()), host, SLOT(slotToolsSettings()));

    d->toolsADLS = new QAction("", host);
    d->toolsADLS->setObjectName("toolsADLS");
    WulforUtil::bindActionIcon(d->toolsADLS, AppIcons::eiADLS);
    QObject::connect(d->toolsADLS, SIGNAL(triggered()), host, SLOT(slotToolsADLS()));

    d->toolsCmdDebug = new QAction("", host);
    d->toolsCmdDebug->setObjectName("toolsCmdDebug");
    WulforUtil::bindActionIcon(d->toolsCmdDebug, AppIcons::eiCONSOLE);
    QObject::connect(d->toolsCmdDebug, SIGNAL(triggered()), host, SLOT(slotToolsCmdDebug()));

    d->toolsSecretary = new QAction("", host);
    d->toolsSecretary->setObjectName("toolsSecretary");
    WulforUtil::bindActionIcon(d->toolsSecretary, AppIcons::eiMAGNET);
    QObject::connect(d->toolsSecretary, SIGNAL(triggered()), host, SLOT(slotToolsSecretary()));

    d->toolsTransfers = new QAction("", host);
    d->toolsTransfers->setObjectName("toolsTransfers");
    SM->registerShortcut(d->toolsTransfers, QString("Ctrl+T"));
    WulforUtil::bindActionIcon(d->toolsTransfers, AppIcons::eiTRANSFER);
    d->toolsTransfers->setCheckable(true);
    QObject::connect(d->toolsTransfers, SIGNAL(toggled(bool)), host, SLOT(slotToolsTransfer(bool)));

    d->toolsDownloadQueue = new QAction("", host);
    d->toolsDownloadQueue->setObjectName("toolsDownloadQueue");
    SM->registerShortcut(d->toolsDownloadQueue, QString("Ctrl+D"));
    WulforUtil::bindActionIcon(d->toolsDownloadQueue, AppIcons::eiDOWNLOAD);
    QObject::connect(d->toolsDownloadQueue, SIGNAL(triggered()), host, SLOT(slotToolsDownloadQueue()));

    d->toolsQueuedUsers = new QAction("", host);
    d->toolsQueuedUsers->setObjectName("toolsQueuedUsers");
    SM->registerShortcut(d->toolsQueuedUsers, QString("Ctrl+Shift+U"));
    WulforUtil::bindActionIcon(d->toolsQueuedUsers, AppIcons::eiUSERS);
    QObject::connect(d->toolsQueuedUsers, SIGNAL(triggered()), host, SLOT(slotToolsQueuedUsers()));

    d->toolsFinishedDownloads = new QAction("", host);
    d->toolsFinishedDownloads->setObjectName("toolsFinishedDownloads");
    SM->registerShortcut(d->toolsFinishedDownloads, QString("Ctrl+["));
    WulforUtil::bindActionIcon(d->toolsFinishedDownloads, AppIcons::eiDOWNLIST);
    QObject::connect(d->toolsFinishedDownloads, SIGNAL(triggered()), host, SLOT(slotToolsFinishedDownloads()));

    d->toolsFinishedUploads = new QAction("", host);
    d->toolsFinishedUploads->setObjectName("toolsFinishedUploads");
    SM->registerShortcut(d->toolsFinishedUploads, QString("Ctrl+]"));
    WulforUtil::bindActionIcon(d->toolsFinishedUploads, AppIcons::eiUPLIST);
    QObject::connect(d->toolsFinishedUploads, SIGNAL(triggered()), host, SLOT(slotToolsFinishedUploads()));

    d->toolsSearchSpy = new QAction("", host);
    d->toolsSearchSpy->setObjectName("toolsSpy");
    WulforUtil::bindActionIcon(d->toolsSearchSpy, AppIcons::eiSPY);
    QObject::connect(d->toolsSearchSpy, SIGNAL(triggered()), host, SLOT(slotToolsSpy()));

    d->toolsAntiSpam = new QAction("", host);
    d->toolsAntiSpam->setObjectName("toolsAntiSpam");
    WulforUtil::bindActionIcon(d->toolsAntiSpam, AppIcons::eiSPAM);
    d->toolsAntiSpam->setCheckable(true);
    d->toolsAntiSpam->setChecked(AntiSpam::getInstance() != nullptr);
    QObject::connect(d->toolsAntiSpam, SIGNAL(triggered()), host, SLOT(slotToolsAntiSpam()));

    d->toolsIPFilter = new QAction("", host);
    d->toolsIPFilter->setObjectName("toolsIPFilter");
    WulforUtil::bindActionIcon(d->toolsIPFilter, AppIcons::eiFILTER);
    d->toolsIPFilter->setCheckable(true);
    d->toolsIPFilter->setChecked(BOOLSETTING(SettingsManager::IPFILTER));
    QObject::connect(d->toolsIPFilter, SIGNAL(triggered()), host, SLOT(slotToolsIPFilter()));

    d->toolsAwayOn = new QAction("", host);
    d->toolsAwayOn->setObjectName("toolsAwayOn");
    d->toolsAwayOn->setCheckable(true);
    QObject::connect(d->toolsAwayOn, SIGNAL(triggered()), host, SLOT(slotToolsSwitchAway()));

    d->toolsAwayOff = new QAction("", host);
    d->toolsAwayOff->setObjectName("toolsAwayOff");
    d->toolsAwayOff->setCheckable(true);
    QObject::connect(d->toolsAwayOff, SIGNAL(triggered()), host, SLOT(slotToolsSwitchAway()));

    d->toolsAutoAway = new QAction("", host);
    d->toolsAutoAway->setCheckable(true);
    d->toolsAutoAway->setChecked(WBGET(WB_APP_AUTO_AWAY));
    QObject::connect(d->toolsAutoAway, SIGNAL(triggered()), host, SLOT(slotToolsAutoAway()));

#ifdef USE_JS
    d->toolsJS = new QAction("", host);
    d->toolsJS->setObjectName("toolsJS");
    WulforUtil::bindActionIcon(d->toolsJS, AppIcons::eiPLUGIN);
    QObject::connect(d->toolsJS, SIGNAL(triggered()), host, SLOT(slotToolsJS()));

    d->toolsJSConsole = new QAction("", host);
    d->toolsJSConsole->setObjectName("toolsJSConsole");
    SM->registerShortcut(d->toolsJSConsole, QString("Ctrl+Alt+J"));
    WulforUtil::bindActionIcon(d->toolsJSConsole, AppIcons::eiCONSOLE);
    QObject::connect(d->toolsJSConsole, SIGNAL(triggered()), host, SLOT(slotToolsJSConsole()));
#endif

    d->menuAwayAction = new QAction("", host);
    QAction *away_sep = new QAction("", host);
    away_sep->setSeparator(true);

    d->awayGroup = new QActionGroup(host);
    d->awayGroup->addAction(d->toolsAwayOn);
    d->awayGroup->addAction(d->toolsAwayOff);

    d->menuAway = new QMenu(host);
    d->menuAway->addActions(QList<QAction*>() << d->toolsAwayOn << d->toolsAwayOff << away_sep << d->toolsAutoAway);
    {
        QAction *act = Util::getAway()? d->toolsAwayOn : d->toolsAwayOff;
        act->setChecked(true);
    }
    d->menuAwayAction->setMenu(d->menuAway);
    d->menuAwayAction->setIcon(WU->getIcon(AppIcons::eiAWAY));

    d->toolsSearch = new QAction("", host);
    d->toolsSearch->setObjectName("toolsSearch");
    SM->registerShortcut(d->toolsSearch, QString("Ctrl+S"));
    WulforUtil::bindActionIcon(d->toolsSearch, AppIcons::eiFILEFIND);
    QObject::connect(d->toolsSearch, SIGNAL(triggered()), host, SLOT(slotToolsSearch()));

    d->toolsHideProgressSpace = new QAction("", host);
    d->toolsHideProgressSpace->setObjectName("toolsHideProgressSpace");

#if (!defined FREE_SPACE_BAR_C)
    d->toolsHideProgressSpace->setVisible(false);
#endif
    WulforUtil::bindActionIcon(d->toolsHideProgressSpace, AppIcons::eiFREESPACE);
    QObject::connect(d->toolsHideProgressSpace, SIGNAL(triggered()), host, SLOT(slotHideProgressSpace()));

    d->toolsHideLastStatus = new QAction("", host);
    d->toolsHideLastStatus->setObjectName("toolsHideLastStatus");
    WulforUtil::bindActionIcon(d->toolsHideLastStatus, AppIcons::eiSTATUS);
    QObject::connect(d->toolsHideLastStatus, SIGNAL(triggered()), host, SLOT(slotHideLastStatus()));

    d->toolsHideUsersStatisctics = new QAction("", host);
    d->toolsHideUsersStatisctics->setObjectName("toolsHideUsersStatisctics");
    WulforUtil::bindActionIcon(d->toolsHideUsersStatisctics, AppIcons::eiUSERS);
    QObject::connect(d->toolsHideUsersStatisctics, SIGNAL(triggered()), host, SLOT(slotHideUsersStatistics()));

    d->toolsSwitchSpeedLimit = new QAction("", host);
    d->toolsSwitchSpeedLimit->setObjectName("toolsSwitchSpeedLimit");
    SM->registerShortcut(d->toolsSwitchSpeedLimit, QString("Ctrl+K"));
    WulforUtil::bindActionIcon(d->toolsSwitchSpeedLimit,
        BOOLSETTING(THROTTLE_ENABLE) ? AppIcons::eiSPEED_LIMIT_ON : AppIcons::eiSPEED_LIMIT_OFF);
    d->toolsSwitchSpeedLimit->setCheckable(true);
    d->toolsSwitchSpeedLimit->setChecked(BOOLSETTING(THROTTLE_ENABLE));
    QObject::connect(d->toolsSwitchSpeedLimit, SIGNAL(triggered()), host, SLOT(slotToolsSwitchSpeedLimit()));

    d->chatClear = new QAction("", host);
    d->chatClear->setObjectName("chatClear");
    WulforUtil::bindActionIcon(d->chatClear, AppIcons::eiCLEAR);
    QObject::connect(d->chatClear, SIGNAL(triggered()), host, SLOT(slotChatClear()));

    d->findInWidget = new QAction("", host);
    d->findInWidget->setObjectName("findInWidget");
    SM->registerShortcut(d->findInWidget, QString("Ctrl+F"));
    WulforUtil::bindActionIcon(d->findInWidget, AppIcons::eiFIND);
    QObject::connect(d->findInWidget, SIGNAL(triggered()), host, SLOT(slotFind()));

    d->chatDisable = new QAction("", host);
    d->chatDisable->setObjectName("chatDisable");
    WulforUtil::bindActionIcon(d->chatDisable, AppIcons::eiEDITDELETE);
    QObject::connect(d->chatDisable, SIGNAL(triggered()), host, SLOT(slotChatDisable()));

    QAction *separator0 = new QAction("", host);
    separator0->setObjectName("separator0");
    separator0->setSeparator(true);
    QAction *separator1 = new QAction("", host);
    separator1->setObjectName("separator1");
    separator1->setSeparator(true);
    QAction *separator2 = new QAction("", host);
    separator2->setObjectName("separator2");
    separator2->setSeparator(true);
    QAction *separator3 = new QAction("", host);
    separator3->setObjectName("separator3");
    separator3->setSeparator(true);
    QAction *separator4 = new QAction("", host);
    separator4->setObjectName("separator4");
    separator4->setSeparator(true);
    QAction *separator5 = new QAction("", host);
    separator5->setObjectName("separator5");
    separator5->setSeparator(true);
    QAction *separator6 = new QAction("", host);
    separator6->setObjectName("separator6");
    separator6->setSeparator(true);

    d->fileMenuActions << d->fileOpenMagnet
            << separator3
            << d->fileFileListBrowser
            << d->fileFileListBrowserLocal
            << d->fileFileListMatchAll
            << d->fileRefreshShareHashProgress
            << separator0
            << d->fileOpenLogFile
            << d->fileOpenDownloadDirectory
            << d->fileFileHasher
            << separator1
            << d->fileHideWindow
            << separator2
            << d->fileQuit;

    d->hubsMenuActions << d->hubsHubReconnect
            << d->hubsQuickConnect
            << d->hubsFavoriteHubs
            << d->hubsPublicHubs
            << separator0
            << d->hubsFavoriteUsers;

    d->toolsMenuActions << d->toolsSearch
            << d->toolsADLS
            << d->toolsSecretary
            << separator0
            << d->toolsTransfers
            << d->toolsDownloadQueue
            << d->toolsQueuedUsers
            << d->toolsFinishedDownloads
            << d->toolsFinishedUploads
            << d->toolsSwitchSpeedLimit
            << separator1
            << d->toolsSearchSpy
            << d->toolsAntiSpam
            << d->toolsIPFilter
            << d->toolsCmdDebug
            << separator2
            << d->menuAwayAction
            << separator3
            << d->toolsHideProgressSpace
            << d->toolsHideLastStatus
            << d->toolsHideUsersStatisctics
#ifdef USE_JS
            << separator6
            << d->toolsJS
            << d->toolsJSConsole
#endif
            << separator4
            << d->toolsCopyWindowTitle
            << separator5
            << d->toolsOptions;

    d->toolBarActions << d->toolsOptions
            << separator0
            << d->fileFileListBrowserLocal
            << d->fileRefreshShareHashProgress
            << separator1
            << d->hubsHubReconnect
            << d->hubsQuickConnect
            << separator2
            << d->hubsFavoriteHubs
            << d->hubsFavoriteUsers
            << d->toolsQueuedUsers
            << d->toolsSearch
            << d->hubsPublicHubs
            << separator3
            << d->toolsTransfers
            << d->toolsDownloadQueue
            << d->toolsFinishedDownloads
            << d->toolsFinishedUploads
            << d->toolsSwitchSpeedLimit
            << separator4
            << d->chatClear
            << d->findInWidget
            << d->chatDisable
            << separator5
            << d->toolsADLS
            << d->toolsSecretary
            << d->toolsSearchSpy
            << d->toolsAntiSpam
            << d->toolsIPFilter
            << separator6
            << d->fileQuit;
}
