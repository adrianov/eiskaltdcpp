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
#include <cstdint>

#include "statusbar/StatusStrip.h"

class QAction;
class QObject;
class QProgressBar;
class QStatusBar;
class QWidget;
class HashProgress;
class Notification;

/**
 * Main-window status strip plus per-second link rates.
 * Samples socket totals, formats compact speeds, and refreshes the strip/tray.
 */
class StatusHud {
public:
    void build(QStatusBar *bar, QWidget *host, QObject *filter);
    bool ready() const { return strip.ready(); }

    /** Core timer: rates, lifetime counters, status map (STATS/DSPEED/…). */
    QMap<QString, QString> sample(uint64_t ticks);

    /** UI thread: strip labels, tray tip, Away action sync. */
    void apply(const QMap<QString, QString> &map, Notification *note,
               QAction *awayOn, QAction *awayOff);

    void setLogMessage(const QString &msg);
    void updateHashing(QAction *refreshAction, HashProgress *dialog);
    void toggleFreeSpace(QAction *action);
    void syncFreeSpaceAction(QAction *action);

    /** Flip prefs and return the new action caption. */
    QString toggleLastStatus(QAction *action);
    QString toggleUsersStats(QAction *action);

    QProgressBar *freeSpaceWidget() const { return strip.freeSpaceWidget(); }
    QProgressBar *hashWidget() const { return strip.hashWidget(); }

private:
    StatusStrip strip;
    uint64_t lastTick = 0;
    uint64_t lastUp = 0;
    uint64_t lastDown = 0;
};
