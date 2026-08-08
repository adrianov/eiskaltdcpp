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

class QAction;
class QObject;
class QProgressBar;
class QStatusBar;
class QWidget;

/**
 * Status-bar gauge for free space on the download volume.
 * Chunk turns yellow below 10 GiB free, red below 1 GiB.
 */
class FreeSpaceBar {
public:
    void build(QWidget *host, QObject *filter);
    void mount(QStatusBar *bar);
    void refresh();
    void toggle(QAction *action);
    void syncAction(QAction *action);
    QProgressBar *widget() const { return bar; }

private:
    QProgressBar *bar = nullptr;
};
