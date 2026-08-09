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
class QCloseEvent;
class QShowEvent;
class QHideEvent;
class QEvent;
class QObject;

/** Exit/unload flags and window show/hide/close handling for the main window. */
class WindowLife {
public:
    bool isUnload = false;
    bool exitBegin = false;

    void setUnload(bool b) { isUnload = b; }
    void beginExit();

    /** After init(): listeners, transfer dock, theme, share cleanup. */
    void boot(MainWindow *host, MainWindowPrivate *d);
    void teardown(MainWindow *host, MainWindowPrivate *d);

    void onClose(MainWindow *host, MainWindowPrivate *d, QCloseEvent *e);
    void onShow(MainWindow *host, MainWindowPrivate *d, QShowEvent *e);
    void onHide(MainWindow *host, MainWindowPrivate *d, QHideEvent *e);
    bool filter(MainWindow *host, MainWindowPrivate *d, QObject *obj, QEvent *e);
};
