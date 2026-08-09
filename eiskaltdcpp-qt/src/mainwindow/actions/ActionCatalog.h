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

class MainWindow;
class MainWindowPrivate;

/** Builds File/Hubs/Tools/Chat QActions and the menu/toolbar action lists. */
class ActionCatalog {
public:
    ActionCatalog(MainWindow *host, MainWindowPrivate *d) : host(host), d(d) {}
    void build();

private:
    MainWindow *host;
    MainWindowPrivate *d;
};
