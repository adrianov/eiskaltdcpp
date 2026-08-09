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

#include "dcpp/stdinc.h"
#include "dcpp/forward.h"

class MainWindow;
class MainWindowPrivate;
class QString;

/** File-menu share actions: lists, hashing, magnets, and log/download folders. */
class WindowShare {
public:
    WindowShare(MainWindow *host, MainWindowPrivate *d) : host(host), d(d) {}

    void browseOwnFiles();
    void browseFilelist();
    void browseOwnFilelist();
    void matchAllLists();
    void showShareBrowser(dcpp::UserPtr usr, const QString &file, const QString &jump_to);
    void openLogFile();
    void openDownloadDirectory();
    void showHashProgress();
    void runFileHasher();
    void refreshShareOrShowHash();
    void openMagnet();

private:
    MainWindow *host;
    MainWindowPrivate *d;
};
