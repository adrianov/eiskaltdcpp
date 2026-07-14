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

#include "PeerConnectFilter.h"
#include "PeerConnectHub.h"
#include "QueueManager.h"

namespace dcpp {

string ClientManager::resolveHubHint(const UserPtr& user, const string& hint) {
    if(!user)
        return Util::emptyString;

    // Prefer historically successful hubs; keep hint only as a soft boost.
    if(OnlineUser* ou = findBestOnlineUser(user->getCID(), hint, false)) {
        const string& url = ou->getClient().getHubUrl();
        if(!url.empty())
            return url;
    }

#ifdef WITH_DHT
    if(findDHTNode(user->getCID()) || user->isSet(User::DHT))
        return "DHT";
#endif

    if(!hint.empty())
        return hint;

    return QueueManager::getInstance()->sourceHubHint(user);
}

OnlineUser* ClientManager::findBestOnlineUser(const CID& cid, const string& hintUrl, bool priv) {
    Lock l(cs);
    OnlineUser* best = nullptr;
    int bestScore = -1;

    OnlinePairC op = onlineUsers.equal_range(cid);
    for(auto i = op.first; i != op.second; ++i) {
        OnlineUser* u = i->second;
        if(!PeerConnectFilter::isViablePeer(*u))
            continue;

        int score = 0;
        const string& hub = u->getClient().getHubUrl();
        if(!hintUrl.empty() && hub == hintUrl)
            score += 10;

        switch(PeerConnectHub::get(u->getUser(), hub)) {
        case PeerConnectHub::SUCCESS: score += 20; break;
        case PeerConnectHub::FAILURE: score -= 10; break;
        default: break;
        }

        const string c = u->getIdentity().getApplication();
        if(!c.empty() && c != "?")
            score += 5;

        if(!isActive(hub) && u->getUser()->isSet(User::PASSIVE)) {
            score += 2;
            if(PeerConnectFilter::prefersRevConnect(*u))
                score += 3;
        } else if(isActive(hub)) {
            score += 1;
        }

        if(score > bestScore) {
            best = u;
            bestScore = score;
        }
    }

    if(best)
        return best;

    if(priv)
        return nullptr;

    return op.first != op.second ? op.first->second : nullptr;
}

} // namespace dcpp
