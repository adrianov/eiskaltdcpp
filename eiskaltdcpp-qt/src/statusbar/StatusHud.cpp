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

#include "statusbar/StatusHud.h"
#include "Notification.h"
#include "WulforSettings.h"
#include "WulforUtil.h"

#include "dcpp/stdinc.h"
#include "dcpp/Client.h"
#include "dcpp/SettingsManager.h"
#include "dcpp/Socket.h"
#include "dcpp/Util.h"

#include <QAction>
#include <QObject>

using namespace dcpp;

namespace {

QString withPerSec(const QString &bytes)
{
    return bytes + QObject::tr("/s");
}

void syncAwayActions(QAction *awayOn, QAction *awayOff)
{
    if (!awayOn || !awayOff)
        return;
    const bool away = Util::getAway();
    if ((away && !awayOn->isChecked()) || (!away && awayOff->isChecked()))
        (away ? awayOn : awayOff)->setChecked(true);
}

} // namespace

void StatusHud::build(QStatusBar *bar, QWidget *host, QObject *filter)
{
    strip.build(bar, host, filter);
}

QMap<QString, QString> StatusHud::sample(uint64_t ticks)
{
    uint64_t diff = ticks - lastTick;
    if (diff < 100U)
        diff = 1U;

    const uint64_t downDiff = Socket::getTotalDown() - lastDown;
    const uint64_t upDiff = Socket::getTotalUp() - lastUp;
    const uint64_t downRate = (downDiff * 1000) / diff;
    const uint64_t upRate = (upDiff * 1000) / diff;

    QMap<QString, QString> map;
    map[QStringLiteral("STATS")] = _q(Client::getCounts());
    map[QStringLiteral("DSPEED")] = WulforUtil::formatDisplayBytes(static_cast<int64_t>(downRate));
    map[QStringLiteral("DOWN")] = WulforUtil::formatBytes(static_cast<int64_t>(Socket::getTotalDown()));
    map[QStringLiteral("USPEED")] = WulforUtil::formatDisplayBytes(static_cast<int64_t>(upRate));
    map[QStringLiteral("UP")] = WulforUtil::formatBytes(static_cast<int64_t>(Socket::getTotalUp()));

    lastTick = ticks;
    lastUp = Socket::getTotalUp();
    lastDown = Socket::getTotalDown();

    SettingsManager *sm = SettingsManager::getInstance();
    sm->set(SettingsManager::TOTAL_UPLOAD, SETTING(TOTAL_UPLOAD) + static_cast<int64_t>(upDiff));
    sm->set(SettingsManager::TOTAL_DOWNLOAD, SETTING(TOTAL_DOWNLOAD) + static_cast<int64_t>(downDiff));

    return map;
}

void StatusHud::apply(const QMap<QString, QString> &map, Notification *note,
                      QAction *awayOn, QAction *awayOff)
{
    if (!strip.ready())
        return;

    strip.updateStats(map);

    if (note) {
        note->setToolTip(withPerSec(map.value(QStringLiteral("DSPEED"))),
                         withPerSec(map.value(QStringLiteral("USPEED"))),
                         map.value(QStringLiteral("DOWN")),
                         map.value(QStringLiteral("UP")));
    }

    syncAwayActions(awayOn, awayOff);
}

void StatusHud::setLogMessage(const QString &msg)
{
    strip.setLogMessage(msg);
}

void StatusHud::updateHashing(QAction *refreshAction, HashProgress *dialog)
{
    strip.updateHashing(refreshAction, dialog);
}

void StatusHud::toggleFreeSpace(QAction *action)
{
    strip.toggleFreeSpace(action);
}

void StatusHud::syncFreeSpaceAction(QAction *action)
{
    strip.syncFreeSpaceAction(action);
}

QString StatusHud::toggleLastStatus(QAction *action)
{
    bool on = WBGET(WB_LAST_STATUS);
    on = !on;
    WBSET(WB_LAST_STATUS, on);
    const QString text = on ? QObject::tr("Hide last status message")
                            : QObject::tr("Show last status message");
    if (action)
        action->setText(text);
    return text;
}

QString StatusHud::toggleUsersStats(QAction *action)
{
    bool on = WBGET(WB_USERS_STATISTICS);
    on = !on;
    WBSET(WB_USERS_STATISTICS, on);
    const QString text = on ? QObject::tr("Hide users statistics")
                            : QObject::tr("Show users statistics");
    if (action)
        action->setText(text);
    return text;
}
