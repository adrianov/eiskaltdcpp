/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "UploadManager.h"

#include "ClientManager.h"
#include "ConnectionManager.h"
#include "FavoriteManager.h"
#include "LogManager.h"
#include "ShareManager.h"
#include "AdcCommand.h"
#include "Upload.h"
#include "UserConnection.h"
#include "extra/ipfilter.h"

namespace dcpp {

void UploadManager::notifyQueuedUsers() {
    Lock l(cs);
    int freeSlots = getFreeSlots()*2;               //because there will be non-connecting users

    //while all contacted users may not connect, many probably will; it's fine that the rest are filled with randomly allocated slots
    while (freeSlots) {
        //get rid of offline users
        while (!waitingUsers.empty() && !waitingUsers.front().first.user->isOnline()) waitingUsers.pop_front();
        if (waitingUsers.empty()) break;                //no users to notify

        // FIXME: record and replay a client url hint URL
        ClientManager::getInstance()->connect(waitingUsers.front().first, Util::toString(Util::rand()));
        --freeSlots;

        waitingUsers.pop_front();
    }
}

void UploadManager::addFailedUpload(const UserConnection& source, string filename) {
    {
        Lock l(cs);
        auto it = find_if(waitingUsers.begin(), waitingUsers.end(), CompareFirst<UserPtr, uint32_t>(source.getUser()));
        if (it==waitingUsers.end()) {
            waitingUsers.emplace_back(source.getHintedUser(), GET_TICK());
        } else {
            it->second = GET_TICK();
        }
        waitingFiles[source.getUser()].insert(filename);        //files for which user's asked
    }

    fire(UploadManagerListener::WaitingAddFile(), source.getHintedUser(), filename);
}

void UploadManager::clearUserFiles(const UserPtr& source) {
    Lock l(cs);
    //run this when a user's got a slot or goes offline.
    auto sit = find_if(waitingUsers.begin(), waitingUsers.end(), CompareFirst<UserPtr, uint32_t>(source));
    if (sit == waitingUsers.end()) return;

    FilesMap::iterator fit = waitingFiles.find(sit->first);
    if (fit != waitingFiles.end()) waitingFiles.erase(fit);
    fire(UploadManagerListener::WaitingRemoveUser(), sit->first);

    waitingUsers.erase(sit);
}

HintedUserList UploadManager::getWaitingUsers() const {
    Lock l(cs);
    HintedUserList u;
    for(auto i = waitingUsers.begin(), iend = waitingUsers.end(); i != iend; ++i) {
        u.push_back(i->first);
    }
    return u;
}

const UploadManager::FileSet& UploadManager::getWaitingUserFiles(const UserPtr& u) {
    Lock l(cs);
    return waitingFiles.find(u)->second;
}

void UploadManager::addConnection(UserConnectionPtr conn) {
    Lock l(cs);
    if (!BOOLSETTING(ALLOW_UPLOAD_MULTI_HUB)) {
        for(auto u: uploads) {
            if (u->getUserConnection().getRemoteIp() == conn->getRemoteIp()) {
                conn->disconnect();
                return;
            }
        }
    }
    if (BOOLSETTING(IPFILTER) && !IPFilter::getInstance()->OK(conn->getRemoteIp(),eDIRECTION_OUT)) {
        conn->error("Your IP is Blocked!");// TODO translate
        LogManager::getInstance()->message(_("IPFilter: Blocked incoming connection to ") + conn->getRemoteIp()); // TODO translate
        //QueueManager::getInstance()->removeSource(conn->getUser(), QueueItem::Source::FLAG_REMOVED);
        conn->disconnect();
        return;
    }

    if(SETTING(REQUIRE_TLS) && !conn->isSecure()) {
        conn->error("Secure connection required!");
        conn->disconnect();
        return;
    }

    conn->addListener(this);
    conn->setState(UserConnection::STATE_GET);
}

void UploadManager::removeConnection(UserConnection* aSource) {
    aSource->removeListener(this);
    if(aSource->isSet(UserConnection::FLAG_HASSLOT)) {
        running--;
        aSource->unsetFlag(UserConnection::FLAG_HASSLOT);
    }
    if(aSource->isSet(UserConnection::FLAG_HASEXTRASLOT)) {
        extra--;
        aSource->unsetFlag(UserConnection::FLAG_HASEXTRASLOT);
    }
}

void UploadManager::reloadRestrictions(){
    limits.RenewList(NULL);
}

} // namespace dcpp
