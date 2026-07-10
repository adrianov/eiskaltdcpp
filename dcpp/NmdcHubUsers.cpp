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

#include "NmdcHub.h"

#include "ClientManager.h"

namespace dcpp {

OnlineUser& NmdcHub::getUser(const string& aNick) {
    OnlineUser* u = NULL;
    {
        Lock l(cs);

        auto i = users.find(aNick);
        if(i != users.end())
            return *i->second;
    }

    UserPtr p;
    if(aNick == getCurrentNick()) {
        p = ClientManager::getInstance()->getMe();
    } else {
        p = ClientManager::getInstance()->getUser(aNick, getHubUrl());
    }

    {
        Lock l(cs);
        u = users.emplace(aNick, new OnlineUser(p, *this, 0)).first->second;
        u->getIdentity().setNick(aNick);
        if(u->getUser() == getMyIdentity().getUser()) {
            setMyIdentity(u->getIdentity());
        }
    }

    ClientManager::getInstance()->putOnline(u);
    return *u;
}

OnlineUser* NmdcHub::findUser(const string& aNick) {
    Lock l(cs);
    auto i = users.find(aNick);
    return i == users.end() ? NULL : i->second;
}

void NmdcHub::putUser(const string& aNick) {
    OnlineUser* ou = NULL;
    {
        Lock l(cs);
        auto i = users.find(aNick);
        if(i == users.end())
            return;
        ou = i->second;
        users.erase(i);
    }
    ClientManager::getInstance()->putOffline(ou);
    delete ou;
}

void NmdcHub::clearUsers() {
    decltype(users) u2;

    {
        Lock l(cs);
        u2.swap(users);
    }

    for(auto& i: u2) {
        ClientManager::getInstance()->putOffline(i.second);
        delete i.second;
    }
}

} // namespace dcpp
