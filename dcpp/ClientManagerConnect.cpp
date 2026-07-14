/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "ClientManager.h"

#include "ConnectionManager.h"
#include "FavoriteManager.h"
#include "LogManager.h"
#include "MappingManager.h"
#include "PeerConnectFilter.h"
#include "PeerConnectLog.h"
#include "PeerConnectTls.h"
#include "QueueManager.h"

namespace dcpp {

OnlineUser* ClientManager::findConnectUser(const HintedUser& user, bool priv) {
    return findBestOnlineUser(user.user->getCID(), user.hint, priv);
}

void ClientManager::markFakeActive(OnlineUser& ou) {
    if(ou.getUser()->isSet(User::PASSIVE))
        return;
    ou.getUser()->setFlag(User::PASSIVE);
    updateUser(ou);
    PeerConnectLog::fakeActiveRetry(ou);
}

bool ClientManager::connect(const HintedUser& user, const string& token, bool reverseConnect, int secureMode) {
    if(!MappingManager::getInstance()->readyForPeerConnect()) {
        dcdebug("ClientManager::connect deferred: waiting for UPnP port mapping\n");
        return false;
    }

    bool priv = FavoriteManager::getInstance()->isPrivate(user.hint);

    Lock l(cs);
    OnlineUser* u = findConnectUser(user, priv);

    if(u && !PeerConnectFilter::isViablePeer(*u)) {
        PeerConnectLog::skip(getNickOrCid(user), user.hint, _("stale or ghost hub user entry"));
        return false;
    }

    if(u && PeerConnectTls::rejectPeer(u->getUser())) {
        PeerConnectLog::skip(getNickOrCid(user), user.hint, _("TLS required but peer does not advertise TLS"));
        return false;
    }

    if(u) {
        if(!ConnectionManager::getInstance()->allowOutgoingConnect(u->getUser())) {
            PeerConnectLog::skip(getNickOrCid(user), user.hint, _("connect cooldown (recent $ConnectToMe)"));
            return false;
        }
        u->getClient().connect(*u, token, reverseConnect, secureMode);
        return true;
    }

    PeerConnectLog::userOffline(user);
    return false;
}

bool ClientManager::wantRevConnect(const HintedUser& user, int attempt) {
    if(attempt < 1)
        return false;

    bool priv = FavoriteManager::getInstance()->isPrivate(user.hint);
    Lock l(cs);
    OnlineUser* ou = findConnectUser(user, priv);
    if(!ou)
        return false;

    if(attempt > 1) {
        if(!isActive())
            return true;
        if(!ou->getUser()->isSet(User::PASSIVE)) {
            markFakeActive(*ou);
            return true;
        }
        return false;
    }

    if(isActive())
        return false;
    if(!ou->getUser()->isSet(User::PASSIVE))
        return false;
    if(ou->getUser()->isSet(User::NAT_TRAVERSAL))
        return true;
    return PeerConnectFilter::prefersRevConnect(*ou);
}

void ClientManager::stopConnect(const HintedUser& user) {
    if(!user.user)
        return;

    user.user->setFlag(User::VIRUS_INFECTED);
    PeerConnectLog::skip(getNickOrCid(user), user.hint, _("user marked as virus-infected"));
    ConnectionManager::getInstance()->blockRetry(user.user);
    QueueManager::getInstance()->removeSource(user.user, QueueItem::Source::FLAG_REMOVED);
    LogManager::getInstance()->message(str(F_("Stopped connecting to %1%: user marked as virus-infected") % getNickOrCid(user)));
}

} // namespace dcpp
