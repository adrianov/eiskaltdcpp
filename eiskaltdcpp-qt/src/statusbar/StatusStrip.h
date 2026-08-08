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

#include <QMap>
#include <QString>

#include "statusbar/FreeSpaceBar.h"
#include "statusbar/HashProgressBar.h"

class QAction;
class QLabel;
class QObject;
class QProgressBar;
class QStatusBar;
class QWidget;
class HashProgress;
class StatusBarLogLabel;

/**
 * Main-window status strip: transfer counters, log line, free disk, hashing.
 */
class StatusStrip {
public:
    void build(QStatusBar *bar, QWidget *host, QObject *filter);
    bool ready() const { return counts != nullptr; }

    void updateStats(const QMap<QString, QString> &map);
    void setLogMessage(const QString &msg);
    void updateHashing(QAction *refreshAction, HashProgress *dialog);
    void toggleFreeSpace(QAction *action);
    void syncFreeSpaceAction(QAction *action);

    QProgressBar *freeSpaceWidget() const { return freeSpace.widget(); }
    QProgressBar *hashWidget() const { return hashing.widget(); }

private:
    QLabel *counts = nullptr;
    QLabel *speed = nullptr;
    QLabel *volume = nullptr;
    StatusBarLogLabel *log = nullptr;
    FreeSpaceBar freeSpace;
    HashProgressBar hashing;
};
