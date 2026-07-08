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

void ClientManager::connect(const HintedUser& user, const string& token, bool reverseConnect, int secureMode) {
    if(!MappingManager::getInstance()->readyForPeerConnect()) {
        dcdebug("ClientManager::connect deferred: waiting for UPnP port mapping\n");
        return;
    }

    bool priv = FavoriteManager::getInstance()->isPrivate(user.hint);

    Lock l(cs);
    OnlineUser* u = findOnlineUser(user, priv);

    if(!u || !PeerConnectFilter::isViablePeer(*u))
        u = findBestOnlineUser(user.user->getCID(), user.hint, priv);

    if(u && !PeerConnectFilter::isViablePeer(*u)) {
        PeerConnectLog::skip(getNickOrCid(user), user.hint, _("stale or ghost hub user entry"));
        return;
    }

    if(u && PeerConnectTls::rejectPeer(u->getUser())) {
        PeerConnectLog::skip(getNickOrCid(user), user.hint, _("TLS required but peer does not advertise TLS"));
        return;
    }

    if(u) {
        u->getClient().connect(*u, token, reverseConnect, secureMode);
    } else {
        PeerConnectLog::userOffline(user);
    }
}

bool ClientManager::wantRevConnect(const HintedUser& user, int attempt) {
    if(attempt < 1)
        return false;
    if(attempt > 1)
        return true;

    if(isActive())
        return false;

    bool priv = FavoriteManager::getInstance()->isPrivate(user.hint);
    Lock l(cs);
    OnlineUser* ou = findOnlineUser(user, priv);
    if(!ou || !ou->getUser()->isSet(User::PASSIVE))
        return false;
    if(ou->getUser()->isSet(User::NAT_TRAVERSAL))
        return true;
    return PeerConnectFilter::prefersRevConnect(*ou);
}

void ClientManager::stopConnect(const HintedUser& user) {
    if(!user.user)
        return;

    PeerConnectLog::skip(getNickOrCid(user), user.hint, _("user marked as virus-infected"));
    ConnectionManager::getInstance()->blockRetry(user.user);
    QueueManager::getInstance()->removeSource(user.user, QueueItem::Source::FLAG_REMOVED);
    LogManager::getInstance()->message(str(F_("Stopped connecting to %1%: user marked as virus-infected") % getNickOrCid(user)));
}

} // namespace dcpp
