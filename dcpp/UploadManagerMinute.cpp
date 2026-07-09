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

namespace dcpp {

void UploadManager::on(TimerManagerListener::Minute, uint64_t /* aTick */) noexcept {
    UserList disconnects;
    {
        Lock l(cs);

        auto i = stable_partition(waitingUsers.begin(), waitingUsers.end(), WaitingUserFresh());
        for (auto j = i; j != waitingUsers.end(); ++j) {
            auto fit = waitingFiles.find(j->first);
            if (fit != waitingFiles.end()) waitingFiles.erase(fit);
            fire(UploadManagerListener::WaitingRemoveUser(), j->first);
        }

        waitingUsers.erase(i, waitingUsers.end());

        if( BOOLSETTING(AUTO_KICK) ) {
            for(auto u: uploads) {
                if(u->getUser()->isOnline()) {
                    u->unsetFlag(Upload::FLAG_PENDING_KICK);
                    continue;
                }

                if(u->isSet(Upload::FLAG_PENDING_KICK)) {
                    disconnects.push_back(u->getUser());
                    continue;
                }

                if(BOOLSETTING(AUTO_KICK_NO_FAVS) && FavoriteManager::getInstance()->isFavoriteUser(u->getUser())) {
                    continue;
                }

                u->setFlag(Upload::FLAG_PENDING_KICK);
            }
        }
    }

    for(auto i = disconnects.begin(); i != disconnects.end(); ++i) {
        LogManager::getInstance()->message(str(F_("Disconnected user leaving the hub: %1%") %
                                               Util::toString(ClientManager::getInstance()->getNicks((*i)->getCID(), Util::emptyString))));
        ConnectionManager::getInstance()->disconnect(*i, false);
    }

    int freeSlots = getFreeSlots();
    if(freeSlots != lastFreeSlots) {
        lastFreeSlots = freeSlots;
        ClientManager::getInstance()->infoUpdated();
    }
}

void UploadManager::on(GetListLength, UserConnection* conn) noexcept {
    conn->error("GetListLength not supported");
    conn->disconnect(false);
}

void UploadManager::on(AdcCommand::GFI, UserConnection* aSource, const AdcCommand& c) noexcept {
    if(aSource->getState() != UserConnection::STATE_GET) {
        dcdebug("UM::onSend Bad state, ignoring\n");
        return;
    }

    if(c.getParameters().size() < 2) {
        aSource->send(AdcCommand(AdcCommand::SEV_RECOVERABLE, AdcCommand::ERROR_PROTOCOL_GENERIC, "Missing parameters"));
        return;
    }

    const string& type = c.getParam(0);
    const string& ident = c.getParam(1);

    if(type == Transfer::names[Transfer::TYPE_FILE]) {
        try {
            aSource->send(ShareManager::getInstance()->getFileInfo(ident));
        } catch(const ShareException&) {
            aSource->fileNotAvail();
        }
    } else {
        aSource->fileNotAvail();
    }
}

void UploadManager::on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) noexcept {
    if(!aUser->isOnline()) {
        clearUserFiles(aUser);
    }
}

} // namespace dcpp
