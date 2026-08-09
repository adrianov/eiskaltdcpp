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

/** Builds docks, wires core signals, and runs the main-window init sequence. */
class WindowSetup {
public:
    WindowSetup(MainWindow *host, MainWindowPrivate *d) : host(host), d(d) {}
    void run();

private:
    MainWindow *host;
    MainWindowPrivate *d;
};
