/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

/** Expanded inside MainWindow's private Q_SLOTS (moc expands macros). */
#define MAINWINDOW_MENU_SLOTS \
    void slotOpenMagnet(); \
    void slotFileOpenLogFile(); \
    void slotFileOpenDownloadDirectory(); \
    void slotFileBrowseFilelist(); \
    void slotFileHasher(); \
    void slotFileBrowseOwnFilelist(); \
    void slotFileMatchAllList(); \
    void slotFileHashProgress(); \
    void slotFileRefreshShareHashProgress(); \
    void slotHubsReconnect(); \
    void slotHubsFavoriteHubs(); \
    void slotHubsPublicHubs(); \
    void slotHubsFavoriteUsers(); \
    void slotToolsDownloadQueue(); \
    void slotToolsQueuedUsers(); \
    void slotToolsHubManager(); \
    void slotToolsFinishedDownloads(); \
    void slotToolsFinishedUploads(); \
    void slotToolsSpy(); \
    void slotToolsAntiSpam(); \
    void slotToolsIPFilter(); \
    void slotToolsSwitchAway(); \
    void slotToolsAutoAway(); \
    void slotToolsSearch(); \
    void slotToolsADLS(); \
    void slotToolsCmdDebug(); \
    void slotToolsSecretary(); \
    void slotToolsCopyWindowTitle(); \
    void slotToolsSettings(); \
    void slotToolsJS(); \
    void slotToolsJSConsole(); \
    void slotJSFileChanged(const QString&); \
    void slotToolsTransfer(bool); \
    void slotToolsSwitchSpeedLimit(); \
    void updateActionIcons(); \
    void slotPanelMenuActionClicked(); \
    void slotWidgetsToggle(); \
    void slotQC(); \
    void slotHideMainMenu(); \
    void slotShowMainMenu(); \
    void slotHideWindow(); \
    void slotHideProgressSpace(); \
    void slotHideLastStatus(); \
    void slotHideUsersStatistics(); \
    void slotSideBarDockMenu(); \
    void slotExit(); \
    void slotToolbarCustomization(); \
    void slotToolbarCustomizerDone(const QList<QAction*> &enabled); \
    void slotCloseCurrentWidget(); \
    void slotUnixSignal(int); \
    void nextMsg(); \
    void prevMsg(); \
    void slotFind(); \
    void slotChatDisable(); \
    void slotAboutOpenUrl(); \
    void slotAboutClient(); \
    void slotAboutQt(); \
    void showShareBrowser(dcpp::UserPtr, const QString &, const QString&); \
    void updateStatus(const QMap<QString,QString> &); \
    void slotUpdateFavHubMenu(); \
    void slotConnectFavHub(QAction*); \
    void slotShowSpeedLimits(); \
    void slotSuppressTxt(); \
    void slotSuppressSnd(); \
    void slotShareIndexQueueEmpty();
