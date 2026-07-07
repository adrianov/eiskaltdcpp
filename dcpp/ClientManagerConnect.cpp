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

#include <unordered_set>

namespace dcpp {

StringList ClientManager::getHubUrls(const CID& cid) const {
    Lock l(cs);
    StringList lst;
    OnlinePairC op = onlineUsers.equal_range(cid);
    for(auto i = op.first; i != op.second; ++i) {
        lst.push_back(i->second->getClient().getHubUrl());
    }
    return lst;
}

StringList ClientManager::getHubs(const CID& cid, const string& hintUrl, bool priv) {
    Lock l(cs);
    StringList lst;
    if(!priv) {
        OnlinePairC op = onlineUsers.equal_range(cid);
        for(auto i = op.first; i != op.second; ++i) {
            lst.push_back(i->second->getClient().getHubUrl());
        }
    } else {
        OnlineUser* u = findOnlineUserHint(cid, hintUrl);
        if(u)
            lst.push_back(u->getClient().getHubUrl());
    }
    return lst;
}

StringList ClientManager::getHubNames(const CID& cid, const string& hintUrl, bool priv) {
    Lock l(cs);
    StringList lst;
    if(!priv) {
        OnlinePairC op = onlineUsers.equal_range(cid);
        for(auto i = op.first; i != op.second; ++i) {
            lst.push_back(i->second->getClient().getHubName());
        }
    } else {
        OnlineUser* u = findOnlineUserHint(cid, hintUrl);
        if(u)
            lst.push_back(u->getClient().getHubName());
    }
    return lst;
}

StringList ClientManager::getNicks(const CID& cid, const string& hintUrl, bool priv) {
    Lock l(cs);
    StringSet ret;
    if(!priv) {
        OnlinePairC op = onlineUsers.equal_range(cid);
        for(auto i = op.first; i != op.second; ++i) {
            ret.insert(i->second->getIdentity().getNick());
        }
    } else {
        OnlineUser* u = findOnlineUserHint(cid, hintUrl);
        if(u)
            ret.insert(u->getIdentity().getNick());
    }
    if(ret.empty()) {
        auto i = nicks.find(cid);
        if(i != nicks.end()) {
            ret.insert(i->second.first);
        } else {
            ret.insert('{' + cid.toBase32() + '}');
        }
    }
    return StringList(ret.begin(), ret.end());
}

void ClientManager::cidsForNick(const string& nick, std::unordered_set<CID>& out) const {
    Lock l(cs);
    for(auto& i : onlineUsers) {
        if(Util::stricmp(i.second->getIdentity().getNick().c_str(), nick.c_str()) == 0)
            out.insert(i.first);
    }
    for(auto& i : nicks) {
        if(Util::stricmp(i.second.first.c_str(), nick.c_str()) == 0)
            out.insert(i.first);
    }
}

OnlineUser* ClientManager::findOnlineUserHint(const CID& cid, const string& hintUrl, OnlinePairC& p) const {
    Lock l(cs);
    p = onlineUsers.equal_range(cid);
    if(p.first == p.second)
        return 0;

    if(!hintUrl.empty()) {
        for(auto i = p.first; i != p.second; ++i) {
            OnlineUser* u = i->second;
            if(u->getClient().getHubUrl() == hintUrl) {
                return u;
            }
        }
    }

    return 0;
}

OnlineUser* ClientManager::findOnlineUser(const HintedUser& user, bool priv) {
    return findOnlineUser(user.user->getCID(), user.hint, priv);
}

OnlineUser* ClientManager::findOnlineUser(const CID& cid, const string& hintUrl, bool priv) {
    Lock l(cs);
    OnlinePairC p;
    OnlineUser* u = findOnlineUserHint(cid, hintUrl, p);
    if(u)
        return u;

    if(p.first == p.second)
        return 0;

    if(priv)
        return 0;

    return p.first->second;
}

void ClientManager::connect(const HintedUser& user, const string& token, bool reverseConnect, int secureMode) {
    if(!MappingManager::getInstance()->readyForPeerConnect()) {
        dcdebug("ClientManager::connect deferred: waiting for UPnP port mapping\n");
        return;
    }

    bool priv = FavoriteManager::getInstance()->isPrivate(user.hint);

    Lock l(cs);
    OnlineUser* u = findOnlineUser(user, priv);

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
