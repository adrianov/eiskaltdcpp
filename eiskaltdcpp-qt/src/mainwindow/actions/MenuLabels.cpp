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

#include "mainwindow/actions/MenuLabels.h"
#include "MainWindowPrivate.h"
#include "WulforSettings.h"

#include <QCoreApplication>

void MenuLabels::retranslate()
{
    //Retranslate menu actions
    {
        d->menuFile->setTitle(QCoreApplication::translate("MainWindow", "&File"));

        d->fileOpenMagnet->setText(QCoreApplication::translate("MainWindow", "Open magnet link"));

        d->fileOpenLogFile->setText(QCoreApplication::translate("MainWindow", "Open log file"));

        d->fileOpenDownloadDirectory->setText(QCoreApplication::translate("MainWindow", "Open download directory"));

        d->fileFileListBrowser->setText(QCoreApplication::translate("MainWindow", "Open filelist..."));

        d->fileFileHasher->setText(QCoreApplication::translate("MainWindow", "Calculate file TTH"));

        d->fileFileListBrowserLocal->setText(QCoreApplication::translate("MainWindow", "Open own filelist"));

        d->fileFileListMatchAll->setText(QCoreApplication::translate("MainWindow", "Match all listings"));

        d->fileRefreshShareHashProgress->setText(QCoreApplication::translate("MainWindow", "Refresh share"));

        d->fileHideWindow->setText(QCoreApplication::translate("MainWindow", "Hide window"));

        if (!WBGET(WB_TRAY_ENABLED))
            d->fileHideWindow->setText(QCoreApplication::translate("MainWindow", "Show/hide find frame"));

        d->fileQuit->setText(QCoreApplication::translate("MainWindow", "Quit"));

        d->menuHubs->setTitle(QCoreApplication::translate("MainWindow", "&Hubs"));

        d->hubsHubReconnect->setText(QCoreApplication::translate("MainWindow", "Reconnect to hub"));

        d->hubsFavoriteHubs->setText(QCoreApplication::translate("MainWindow", "Favourite hubs"));

        d->hubsPublicHubs->setText(QCoreApplication::translate("MainWindow", "Public hubs"));

        d->hubsFavoriteUsers->setText(QCoreApplication::translate("MainWindow", "Favourite users"));

        d->hubsQuickConnect->setText(QCoreApplication::translate("MainWindow", "Quick connect"));

        d->menuTools->setTitle(QCoreApplication::translate("MainWindow", "&Tools"));

        d->toolsTransfers->setText(QCoreApplication::translate("MainWindow", "Transfers"));

        d->toolsDownloadQueue->setText(QCoreApplication::translate("MainWindow", "Download queue"));

        d->toolsQueuedUsers->setText(QCoreApplication::translate("MainWindow", "Queued Users"));

        d->toolsHubManager->setText(QCoreApplication::translate("MainWindow", "Hub Manager"));

        d->toolsFinishedDownloads->setText(QCoreApplication::translate("MainWindow", "Finished downloads"));

        d->toolsFinishedUploads->setText(QCoreApplication::translate("MainWindow", "Finished uploads"));

        d->toolsSearchSpy->setText(QCoreApplication::translate("MainWindow", "Search Spy"));

        d->toolsAntiSpam->setText(QCoreApplication::translate("MainWindow", "AntiSpam module"));

        d->toolsIPFilter->setText(QCoreApplication::translate("MainWindow", "IPFilter module"));

        d->status.syncFreeSpaceAction(d->toolsHideProgressSpace);

        d->toolsHideLastStatus->setText(QCoreApplication::translate("MainWindow", "Hide last status message"));

        if (!WBGET(WB_LAST_STATUS))
            d->toolsHideLastStatus->setText(QCoreApplication::translate("MainWindow", "Show last status message"));

        d->toolsHideUsersStatisctics->setText(QCoreApplication::translate("MainWindow", "Hide users statistics"));

        if (!WBGET(WB_USERS_STATISTICS))
            d->toolsHideUsersStatisctics->setText(QCoreApplication::translate("MainWindow", "Show users statistics"));

        d->menuAway->setTitle(QCoreApplication::translate("MainWindow", "Away message"));

        d->toolsAwayOn->setText(QCoreApplication::translate("MainWindow", "On"));

        d->toolsAwayOff->setText(QCoreApplication::translate("MainWindow", "Off"));

        d->toolsAutoAway->setText(QCoreApplication::translate("MainWindow", "Away when not visible"));

        d->toolsCopyWindowTitle->setText(QCoreApplication::translate("MainWindow", "Copy window title"));

        d->toolsOptions->setText(QCoreApplication::translate("MainWindow", "Preferences"));

        d->toolsSearch->setText(QCoreApplication::translate("MainWindow", "Search"));

        d->toolsADLS->setText(QCoreApplication::translate("MainWindow", "ADLSearch"));

        d->toolsCmdDebug->setText(QCoreApplication::translate("MainWindow", "Debug Console"));

        d->toolsSecretary->setText(QCoreApplication::translate("MainWindow", "Secretary"));

        d->toolsSwitchSpeedLimit->setText(QCoreApplication::translate("MainWindow", "Speed limit On/Off"));

#ifdef USE_JS
        d->toolsJS->setText(QCoreApplication::translate("MainWindow", "Scripts Manager"));

        d->toolsJSConsole->setText(QCoreApplication::translate("MainWindow", "Script Console"));
#endif

        d->chatClear->setText(QCoreApplication::translate("MainWindow", "Clear chat"));

        d->findInWidget->setText(QCoreApplication::translate("MainWindow", "Find/Filter"));

        d->chatDisable->setText(QCoreApplication::translate("MainWindow", "Disable/enable chat"));

        d->menuWidgets->setTitle(QCoreApplication::translate("MainWindow", "&Widgets"));

        d->menuPanels->setTitle(QCoreApplication::translate("MainWindow", "&Panels"));

        if (!WBGET(WB_MAINWINDOW_USE_SIDEBAR))
            d->panelsWidgets->setText(QCoreApplication::translate("MainWindow", "Widgets panel"));
        else
            d->panelsWidgets->setText(QCoreApplication::translate("MainWindow", "Widgets side dock"));

        d->panelsTools->setText(QCoreApplication::translate("MainWindow", "Tools panel"));

        d->panelsSearch->setText(QCoreApplication::translate("MainWindow", "Fast search panel"));

        d->menuAbout->setTitle(QCoreApplication::translate("MainWindow", "&Help"));

        d->aboutHomepage->setText(QCoreApplication::translate("MainWindow", "Homepage"));

        d->aboutBuilds->setText(QCoreApplication::translate("MainWindow", "Download program"));

        d->aboutIssues->setText(QCoreApplication::translate("MainWindow", "Report a Bug"));

        d->aboutWiki->setText(QCoreApplication::translate("MainWindow", "Wiki of project"));

        d->aboutChangelog->setText(QCoreApplication::translate("MainWindow", "Changelog (git)"));

        d->aboutSource->setText(QCoreApplication::translate("MainWindow", "Source code (git)"));

        d->aboutClient->setText(QCoreApplication::translate("MainWindow", "About EiskaltDC++"));

        d->aboutQt->setText(QCoreApplication::translate("MainWindow", "About Qt"));
    }
    {
        d->sh_menu->setTitle(QCoreApplication::translate("MainWindow", "Actions"));
    }
    {
        d->arena->setWindowTitle(QCoreApplication::translate("MainWindow", "Main layout"));
    }
}

