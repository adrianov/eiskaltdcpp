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
#include "FavoriteManager.h"

#include "ClientManager.h"

namespace dcpp {

void FavoriteManager::userUpdated(const OnlineUser& info) {
    Lock l(cs);
    auto i = users.find(info.getUser()->getCID());
    if(i != users.end()) {
        FavoriteUser& fu = i->second;
        fu.update(info);
        save();
    }
}

FavoriteHubEntryPtr FavoriteManager::getFavoriteHubEntry(const string& aServer) const {
    for(auto i = favoriteHubs.begin(), iend = favoriteHubs.end(); i != iend; ++i) {
        FavoriteHubEntry* hub = *i;
        if(Util::stricmp(hub->getServer(), aServer) == 0) {
            return hub;
        }
    }
    return NULL;
}

string FavoriteManager::getHubNick(const string& url) const {
    Lock l(cs);
    auto i = hubNicks.find(url);
    return i != hubNicks.end() ? i->second : Util::emptyString;
}

void FavoriteManager::setHubNick(const string& url, const string& nick) {
    Lock l(cs);
    hubNicks[url] = nick;
}

string FavoriteManager::nextHubNick(const string& url, const string& currentNick) const {
    string base;
    if(FavoriteHubEntryPtr fav = getFavoriteHubEntry(url)) {
        base = fav->getNick(false);
    }
    if(base.empty())
        base = SETTING(NICK);

    int suffix = 0;
    if(currentNick.size() > base.size() && Util::stricmp(currentNick.substr(0, base.size()), base) == 0) {
        suffix = Util::toInt(currentNick.substr(base.size()));
        if(suffix <= 0)
            return Util::emptyString;
    } else if(Util::stricmp(currentNick, base) != 0) {
        base = currentNick;
        suffix = 0;
    }

    if(suffix >= 999)
        return Util::emptyString;

    const string suf = Util::toString(suffix + 1);
    string nick = base;
    if(nick.size() + suf.size() > 35)
        nick = nick.substr(0, 35 - suf.size());
    if(nick.empty())
        return Util::emptyString;
    return nick + suf;
}

FavoriteHubEntryList FavoriteManager::getFavoriteHubs(const string& group) const {
    FavoriteHubEntryList ret;
    for(auto i = favoriteHubs.begin(), iend = favoriteHubs.end(); i != iend; ++i)
        if((*i)->getGroup() == group)
            ret.push_back(*i);
    return ret;
}

bool FavoriteManager::isPrivate(const string& url) const {
    if(url.empty())
        return false;

    FavoriteHubEntry* fav = getFavoriteHubEntry(url);
    if(fav) {
        const string& name = fav->getGroup();
        if(!name.empty()) {
            auto group = favHubGroups.find(name);
            if(group != favHubGroups.end())
                return group->second.priv;
        }
    }
    return false;
}

bool FavoriteManager::hasSlot(const UserPtr& aUser) const {
    Lock l(cs);
    auto i = users.find(aUser->getCID());
    if(i == users.end())
        return false;
    return i->second.isSet(FavoriteUser::FLAG_GRANTSLOT);
}

time_t FavoriteManager::getLastSeen(const UserPtr& aUser) const {
    Lock l(cs);
    auto i = users.find(aUser->getCID());
    if(i == users.end())
        return 0;
    return i->second.getLastSeen();
}

void FavoriteManager::setAutoGrant(const UserPtr& aUser, bool grant) {
    Lock l(cs);
    auto i = users.find(aUser->getCID());
    if(i == users.end())
        return;
    if(grant)
        i->second.setFlag(FavoriteUser::FLAG_GRANTSLOT);
    else
        i->second.unsetFlag(FavoriteUser::FLAG_GRANTSLOT);
    save();
}
void FavoriteManager::setUserDescription(const UserPtr& aUser, const string& description) {
    Lock l(cs);
    auto i = users.find(aUser->getCID());
    if(i == users.end())
        return;
    i->second.setDescription(description);
    save();
}


} // namespace dcpp
