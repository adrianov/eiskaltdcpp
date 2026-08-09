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

#include <QList>

class MainWindow;
class MainWindowPrivate;
class QAction;

/** Builds menu bar, toolbars, search bar, and side dock chrome; runs panel/toolbar slots. */
class WindowChrome {
public:
    WindowChrome(MainWindow *host, MainWindowPrivate *d) : host(host), d(d) {}
    void initMenuBar();
    void initSearchBar();
    void initToolbar();
    void initSideBar();

    void addToolAction(QAction *act);
    void remToolAction(QAction *act);
    void toggleMainMenu(bool showMenu);
    void hideMainMenu();
    void showCompactMenu();
    void panelMenuClicked();
    void customizeToolbar();
    void applyToolbarActions(const QList<QAction*> &enabled);
    void sideBarDockMenu();

private:
    MainWindow *host;
    MainWindowPrivate *d;
};
