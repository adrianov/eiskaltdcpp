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
#include "SettingsManager.h"
#include "UploadManager.h"
#include "UserConnection.h"

namespace dcpp {

void ConnectionManager::addDownloadConnection(UserConnection* uc) {
    bool addConn = false;
    {
        Lock l(cs);

        auto* cqi = findDownloadCqi(uc->getHintedUser());
        if(cqi) {
            // Never bind a download to a CQI from another hub (wrong file list / identity).
            if(uc->getUser() && uc->getUser()->isSet(User::NMDC) &&
                    !cqi->getUser().hint.empty() && !uc->getHubUrl().empty() &&
                    cqi->getUser().hint != uc->getHubUrl()) {
                cqi = nullptr;
            } else if(slotWaitActive(cqi)) {
                putConnection(uc);
                return;
            } else if(cqi->getState() == ConnectionQueueItem::WAITING ||
                    cqi->getState() == ConnectionQueueItem::CONNECTING) {
                cqi->setState(ConnectionQueueItem::ACTIVE);
                cqi->setSlotWaits(0);
                cqi->setErrors(0);
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

namespace {

/** Idle upload waiting for $Get/$ADCGET — safe to replace on peer reconnect spam. */
UserConnection* findIdleUpload(UserConnectionList& conns, UserConnection* uc) {
    for(auto* existing : conns) {
        if(existing == uc || existing->getUser() != uc->getUser())
            continue;
        if(!existing->isSet(UserConnection::FLAG_UPLOAD) ||
                !existing->isSet(UserConnection::FLAG_ASSOCIATED))
            continue;
        if(existing->getState() == UserConnection::STATE_GET)
            return existing;
    }
    return nullptr;
}

} // namespace

void ConnectionManager::addUploadConnection(UserConnection* uc) {
    dcassert(uc->isSet(UserConnection::FLAG_UPLOAD));

    bool addConn = false;
    UserConnection* stale = nullptr;
    {
        Lock l(cs);

        const bool haveUpload = find(uploads.begin(), uploads.end(), uc->getUser()) != uploads.end();
        stale = findIdleUpload(userConnections, uc);

        if(!haveUpload || (!stale && BOOLSETTING(ALLOW_SIM_UPLOADS))) {
            ConnectionQueueItem* cqi = getCQI(uc->getHintedUser(), false);
            cqi->setState(ConnectionQueueItem::ACTIVE);
            uc->setFlag(UserConnection::FLAG_ASSOCIATED);
            fire(ConnectionManagerListener::Connected(), cqi);
            PeerConnectLog::connected(cqi->getUser(), false);
            addConn = true;
        } else if(stale) {
            // Reconnect while a prior socket sat idle — reuse the slot.
            stale->unsetFlag(UserConnection::FLAG_ASSOCIATED);
            uc->setFlag(UserConnection::FLAG_ASSOCIATED);
            addConn = true;
        }
    }

    if(stale)
        stale->disconnect(true);

    if(addConn) {
        UploadManager::getInstance()->addConnection(uc);
    } else {
        rejectConnection(uc, _("upload already active for user"));
    }
}

} // namespace dcpp
