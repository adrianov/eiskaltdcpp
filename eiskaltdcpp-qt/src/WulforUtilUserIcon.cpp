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

#include "WulforUtil.h"

#include "dcpp/ClientManager.h"
#include "dcpp/User.h"
#include "dcpp/CID.h"
#include "dcpp/AdcHub.h"
#include "dcpp/OnlineUser.h"

#include <QtGlobal>

using namespace dcpp;

bool WulforUtil::loadUserIconsFromFile(QString file){
    // Sheet is USERLIST_XPM_COLUMNS × USERLIST_XPM_ROWS square cells.
    // Physical cell size may be larger than USERLIST_ICON_SIZE (Retina source);
    // getUserIcon() crops by cell then scalePixmap() down to logical pixels.
    QImage img;
    if (!img.load(file, "PNG"))
        return false;

    if (img.width() % USERLIST_XPM_COLUMNS
            || img.height() % USERLIST_XPM_ROWS) {
        qWarning("usericons: %s size %dx%d is not divisible by %dx%d grid",
                 qPrintable(file), img.width(), img.height(),
                 USERLIST_XPM_COLUMNS, USERLIST_XPM_ROWS);
        return false;
    }

    const int cellW = img.width() / USERLIST_XPM_COLUMNS;
    const int cellH = img.height() / USERLIST_XPM_ROWS;
    if (cellW != cellH) {
        qWarning("usericons: %s cells are %dx%d (want square)",
                 qPrintable(file), cellW, cellH);
        return false;
    }

    *userIcons = img;
    clearUserIconCache();
    return true;
}

void WulforUtil::clearUserIconCache(){
    for (int x = 0; x < USERLIST_XPM_COLUMNS; ++x) {
        for (int y = 0; y < USERLIST_XPM_ROWS; ++y) {
            if (userIconCache[x][y]) {
                delete userIconCache[x][y];
                userIconCache[x][y] = nullptr;
            }
        }
    }
}

QPixmap *WulforUtil::getUserIcon(const UserPtr &id, bool isAway, bool isOp, const QString &sp){

    int x = connectionSpeeds.value(sp, 5);
    int y = 0;

    if (isAway)
        y += 1;

    if (id->isSet(User::TLS))
        y += 2;

    Identity iid = ClientManager::getInstance()->getOnlineUserIdentity(id);

    if( (iid.supports(AdcHub::ADCS_FEATURE) && iid.supports(AdcHub::SEGA_FEATURE)) &&
        ((iid.supports(AdcHub::TCP4_FEATURE) && iid.supports(AdcHub::UDP4_FEATURE)) || iid.supports(AdcHub::NAT0_FEATURE)))
        y += 4;

    if (isOp)
        y += 8;

    if (id->isSet(User::PASSIVE)){
        y += 16;

        if (SETTING(INCOMING_CONNECTIONS) == SettingsManager::INCOMING_FIREWALL_PASSIVE)
            x = 7;
    }

    if (userIconCache[x][y] == nullptr) {
        const int cell = userIcons->width() / USERLIST_XPM_COLUMNS;
        userIconCache[x][y] = new QPixmap(scalePixmap(
                QPixmap::fromImage(userIcons->copy(
                        x * cell, y * cell, cell, cell)),
                USERLIST_ICON_SIZE));
    }

    return userIconCache[x][y];
}
