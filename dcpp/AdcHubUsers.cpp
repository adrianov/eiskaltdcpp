/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ADC online-user map helpers.
 */

#include "stdinc.h"
#include "AdcHub.h"

#include "ClientManager.h"

namespace dcpp {

OnlineUser& AdcHub::getUser(const uint32_t aSID, const CID& aCID) {
    OnlineUser* ou = findUser(aSID);
    if(ou) {
        return *ou;
    }

    UserPtr p = ClientManager::getInstance()->getUser(aCID);

    {
        Lock l(cs);
        ou = users.emplace(aSID, new OnlineUser(p, *this, aSID)).first->second;
    }

    if(aSID != AdcCommand::HUB_SID)
        ClientManager::getInstance()->putOnline(ou);
    return *ou;
}

OnlineUser* AdcHub::findUser(const uint32_t aSID) const {
    Lock l(cs);
    auto i = users.find(aSID);
    return i == users.end() ? NULL : i->second;
}

OnlineUser* AdcHub::findUser(const CID& aCID) const {
    Lock l(cs);
    for(auto& i: users) {
        if(i.second->getUser()->getCID() == aCID) {
            return i.second;
        }
    }
    return 0;
}

void AdcHub::putUser(const uint32_t aSID, bool disconnect) {
    OnlineUser* ou = nullptr;
    {
        Lock l(cs);
        auto i = users.find(aSID);
        if(i == users.end())
            return;
        ou = i->second;
        users.erase(i);
    }

    if(aSID != AdcCommand::HUB_SID)
        ClientManager::getInstance()->putOffline(ou, disconnect);

    fire(ClientListener::UserRemoved(), this, *ou);
    delete ou;
}

void AdcHub::clearUsers() {
    decltype(users) tmp;
    {
        Lock l(cs);
        users.swap(tmp);
    }

    for(auto& i: tmp) {
        if(i.first != AdcCommand::HUB_SID)
            ClientManager::getInstance()->putOffline(i.second);
        delete i.second;
    }
}

} // namespace dcpp
