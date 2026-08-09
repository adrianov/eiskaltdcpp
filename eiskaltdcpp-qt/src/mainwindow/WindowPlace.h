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
class QWidget;

/** Remembers and restores main-window size, position, and maximized state. */
class WindowPlace {
public:
    bool showMax = false;
    int w = 800;
    int h = 600;
    int xPos = 0;
    int yPos = 0;

    void load(MainWindow *host, MainWindowPrivate *d);
    void save(MainWindow *host, MainWindowPrivate *d);
    void capture(MainWindow *host);
    void applyShow(MainWindow *host);
};
