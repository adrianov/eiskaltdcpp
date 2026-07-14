/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "ConnectionManager.h"

#include "MappingManager.h"
#include "PeerConnectLog.h"

namespace dcpp {

ConnectionQueueItem* ConnectionManager::getCQI(const HintedUser& user, bool download) {
    ConnectionQueueItem* cqi = new ConnectionQueueItem(user, download);
    if(download) {
        dcassert(find(downloads.begin(), downloads.end(), user.user) == downloads.end());
        downloads.push_back(cqi);
    } else {
        // Multiple upload CQIs per user are allowed when ALLOW_SIM_UPLOADS is on.
        uploads.push_back(cqi);
    }

    fire(ConnectionManagerListener::Added(), cqi);
    if(download)
        PeerConnectLog::queueAdded(user);
    return cqi;
}

void ConnectionManager::putCQI(ConnectionQueueItem* cqi) {
    fire(ConnectionManagerListener::Removed(), cqi);
    if(cqi->getDownload()) {
        dcassert(find(downloads.begin(), downloads.end(), cqi) != downloads.end());
        downloads.erase(remove(downloads.begin(), downloads.end(), cqi), downloads.end());
    } else {
        dcassert(find(uploads.begin(), uploads.end(), cqi) != uploads.end());
        uploads.erase(remove(uploads.begin(), uploads.end(), cqi), uploads.end());
    }
    delete cqi;
}

bool ConnectionManager::isQueuedForDownload(const UserPtr& user) const {
    if(!user)
        return false;
    Lock l(cs);
    for(auto& cqi: downloads) {
        if(cqi->getUser().user != user)
            continue;
        if(cqi->getState() == ConnectionQueueItem::ACTIVE || cqi->getErrors() != -1)
            return true;
    }
    return false;
}

QueuedDownloadUsers ConnectionManager::queuedDownloadUsers() const {
    Lock l(cs);
    QueuedDownloadUsers out;
    for(auto& cqi: downloads) {
        if(cqi->getState() == ConnectionQueueItem::ACTIVE || cqi->getErrors() != -1)
            out.insert(cqi->getUser().user->getCID());
    }
    return out;
}

void ConnectionManager::onUpnpReady() {
    Lock l(cs);
    for(auto& cqi : downloads) {
        // Do not wipe slot-wait / error backoff when UPnP remaps.
        if(cqi->getState() != ConnectionQueueItem::ACTIVE && !queueBackoffActive(cqi))
            cqi->setLastAttempt(0);
    }
}

} // namespace dcpp
