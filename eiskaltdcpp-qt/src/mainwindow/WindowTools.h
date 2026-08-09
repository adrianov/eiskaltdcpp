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

#include "ArenaWidget.h"

class MainWindow;
class MainWindowPrivate;
class QString;

/** Tools-menu handlers: arena toggles, settings, away, scripts, transfer dock. */
class WindowTools {
public:
    WindowTools(MainWindow *host, MainWindowPrivate *d) : host(host), d(d) {}

    void toggleRole(ArenaWidget::Role role);
    void toggleSingleton(ArenaWidget *a);
    void openSearch();
    void openAntiSpam();
    void openIpFilter();
    void setAutoAway();
    void switchAway();
    void openScriptManager();
    void scriptFileChanged(const QString &script);
    void openScriptConsole();
    void openSettings();
    void toggleTransferDock(bool toggled);
    void switchSpeedLimit();
    void copyWindowTitle();
    void quickConnect();
    void reloadSettings();

private:
    MainWindow *host;
    MainWindowPrivate *d;
};
