/***************************************************************************
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

using namespace dcpp;

bool WulforUtil::loadUserIconsFromFile(QString file){
    if (userIcons->load(file, "PNG")){
        clearUserIconCache();

        return true;
    }

    return false;
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



// Render a pixmap at the physical resolution of the screen so icons stay sharp on Retina.
