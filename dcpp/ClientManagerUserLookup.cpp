/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "ClientManager.h"

#include "ConnectionQueueItem.h"
#include "TigerHash.h"

namespace dcpp {

UserPtr ClientManager::findLegacyUser(const string& aNick) const noexcept {
    if(aNick.empty())
        return UserPtr();
    Lock l(cs);

    for(auto& i: onlineUsers) {
        const OnlineUser* ou = i.second;
        if(ou->getUser()->isSet(User::NMDC) && Util::stricmp(ou->getIdentity().getNick(), aNick) == 0)
            return ou->getUser();
    }
    return UserPtr();
}

UserPtr ClientManager::findNmdcUser(const string& utf8Nick, const string& hubUrl) const noexcept {
    if(utf8Nick.empty() || hubUrl.empty())
        return UserPtr();

    if(UserPtr u = findUser(utf8Nick, hubUrl))
        return u;

    string matchedNick;
    {
        Lock l(cs);
        for(auto& i : onlineUsers) {
            const OnlineUser* ou = i.second;
            if(!ou->getUser()->isSet(User::NMDC) || ou->getClient().getHubUrl() != hubUrl)
                continue;
            const string& hubNick = ou->getIdentity().getNick();
            if(!nickWireMatch(utf8Nick, hubNick))
                continue;
            if(hubNick.size() > matchedNick.size())
                matchedNick = hubNick;
        }
    }

    if(!matchedNick.empty())
        return findUser(matchedNick, hubUrl);

    const string stripped = stripWireCountryTag(utf8Nick);
    if(stripped != utf8Nick)
        return findUser(stripped, hubUrl);

    return UserPtr();
}

UserPtr ClientManager::getUser(const string& aNick, const string& aHubUrl) noexcept {
    CID cid = makeCid(aNick, aHubUrl);
    Lock l(cs);

    auto ui = users.find(cid);
    if(ui != users.end()) {
        ui->second->setFlag(User::NMDC);
        return ui->second;
    }

    UserPtr p(new User(cid));
    p->setFlag(User::NMDC);
    users.emplace(cid, p);

    return p;
}

UserPtr ClientManager::getUser(const CID& cid) noexcept {
    Lock l(cs);
    auto ui = users.find(cid);
    if(ui != users.end()) {
        return ui->second;
    }

    if(cid == getMe()->getCID()) {
        return getMe();
    }

    UserPtr p(new User(cid));
    users.emplace(cid, p);
    return p;
}

UserPtr ClientManager::findUser(const CID& cid) const noexcept {
    Lock l(cs);
    auto ui = users.find(cid);
    return ui == users.end() ? nullptr : ui->second;
}

bool ClientManager::isOp(const UserPtr& user, const string& aHubUrl) const {
    Lock l(cs);
    OnlinePairC p = onlineUsers.equal_range(user->getCID());
    for(auto i = p.first; i != p.second; ++i) {
        if(i->second->getClient().getHubUrl() == aHubUrl) {
            return i->second->getIdentity().isOp();
        }
    }
    return false;
}

CID ClientManager::makeCid(const string& aNick, const string& aHubUrl) const noexcept {
    string n = Text::toLower(aNick);
    TigerHash th;
    th.update(n.c_str(), n.length());
    th.update(Text::toLower(aHubUrl).c_str(), aHubUrl.length());
    return CID(th.finalize());
}

} // namespace dcpp
