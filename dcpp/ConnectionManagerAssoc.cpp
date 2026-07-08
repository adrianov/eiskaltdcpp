/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "ConnectionManager.h"

#include "DownloadManager.h"
#include "PeerConnectLog.h"
#include "UploadManager.h"
#include "UserConnection.h"

namespace dcpp {

void ConnectionManager::addDownloadConnection(UserConnection* uc) {
    bool addConn = false;
    {
        Lock l(cs);

        auto i = find(downloads.begin(), downloads.end(), uc->getUser());
        if(i != downloads.end()) {
            auto& cqi = *i;
            if(cqi->getState() == ConnectionQueueItem::WAITING || cqi->getState() == ConnectionQueueItem::CONNECTING) {
                cqi->setState(ConnectionQueueItem::ACTIVE);
                cqi->setSlotWaits(0);
                uc->setFlag(UserConnection::FLAG_ASSOCIATED);

                fire(ConnectionManagerListener::Connected(), cqi);

                PeerConnectLog::connected(cqi->getUser(), true);
                dcdebug("ConnectionManager::addDownloadConnection, leaving to downloadmanager\n");
                addConn = true;
            }
        }
    }

    if(addConn) {
        DownloadManager::getInstance()->addConnection(uc);
    } else {
        rejectConnection(uc, _("download queue item not in waiting/connecting state"));
    }
}

void ConnectionManager::addUploadConnection(UserConnection* uc) {
    dcassert(uc->isSet(UserConnection::FLAG_UPLOAD));

    bool addConn = false;
    {
        Lock l(cs);

        auto i = find(uploads.begin(), uploads.end(), uc->getUser());
        if(i == uploads.end()) {
            ConnectionQueueItem* cqi = getCQI(uc->getHintedUser(), false);

            cqi->setState(ConnectionQueueItem::ACTIVE);
            uc->setFlag(UserConnection::FLAG_ASSOCIATED);

            fire(ConnectionManagerListener::Connected(), cqi);

            PeerConnectLog::connected(cqi->getUser(), false);
            dcdebug("ConnectionManager::addUploadConnection, leaving to uploadmanager\n");
            addConn = true;
        }
    }

    if(addConn) {
        UploadManager::getInstance()->addConnection(uc);
    } else {
        rejectConnection(uc, _("upload already active for user"));
    }
}

} // namespace dcpp
