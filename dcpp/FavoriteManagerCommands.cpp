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

UserCommand FavoriteManager::addUserCommand(int type, int ctx, int flags, const string& name, const string& command, const string& to, const string& hub) {
    // No dupes, add it...
    Lock l(cs);
    userCommands.push_back(UserCommand(lastId++, type, ctx, flags, name, command, to, hub));
    UserCommand& uc = userCommands.back();
    if(!uc.isSet(UserCommand::FLAG_NOSAVE))
        save();
    return userCommands.back();
}

bool FavoriteManager::getUserCommand(int cid, UserCommand& uc) {
    Lock l(cs);
    for(auto& i: userCommands) {
        if(i.getId() == cid) {
            uc = i;
            return true;
        }
    }
    return false;
}

bool FavoriteManager::moveUserCommand(int cid, int pos) {
    dcassert(pos == -1 || pos == 1);
    Lock l(cs);
    for(auto i = userCommands.begin(); i != userCommands.end(); ++i) {
        if(i->getId() == cid) {
            std::swap(*i, *(i + pos));
            return true;
        }
    }
    return false;
}

void FavoriteManager::updateUserCommand(const UserCommand& uc) {
    bool nosave = true;
    Lock l(cs);
    for(auto& i: userCommands) {
        if(i.getId() == uc.getId()) {
            i = uc;
            nosave = uc.isSet(UserCommand::FLAG_NOSAVE);
            break;
        }
    }
    if(!nosave)
        save();
}

int FavoriteManager::findUserCommand(const string& aName, const string& aUrl) {
    Lock l(cs);
    for(auto& i: userCommands) {
        if(i.getName() == aName && i.getHub() == aUrl) {
            return i.getId();
        }
    }
    return -1;
}

void FavoriteManager::removeUserCommand(int cid) {
    bool nosave = true;
    Lock l(cs);
    for(auto i = userCommands.begin(); i != userCommands.end(); ++i) {
        if(i->getId() == cid) {
            nosave = i->isSet(UserCommand::FLAG_NOSAVE);
            userCommands.erase(i);
            break;
        }
    }
    if(!nosave)
        save();
}
void FavoriteManager::removeUserCommand(const string& srv) {
    Lock l(cs);
    userCommands.erase(std::remove_if(userCommands.begin(), userCommands.end(), [&](const UserCommand& uc) {
        return uc.getHub() == srv && uc.isSet(UserCommand::FLAG_NOSAVE);
    }), userCommands.end());
}

void FavoriteManager::removeHubUserCommands(int ctx, const string& hub) {
    Lock l(cs);
    userCommands.erase(std::remove_if(userCommands.begin(), userCommands.end(), [&](const UserCommand& uc) {
        return uc.getHub() == hub && uc.isSet(UserCommand::FLAG_NOSAVE) && uc.getCtx() & ctx;
    }), userCommands.end());
}


UserCommand::List FavoriteManager::getUserCommands(int ctx, const StringList& hubs) {
    vector<bool> isOp(hubs.size());

    for(size_t i = 0; i < hubs.size(); ++i) {
        if(ClientManager::getInstance()->isOp(ClientManager::getInstance()->getMe(), hubs[i])) {
            isOp[i] = true;
        }
    }

    Lock l(cs);
    UserCommand::List lst;
    for(auto i = userCommands.begin(); i != userCommands.end(); ++i) {
        UserCommand& uc = *i;
        if(!(uc.getCtx() & ctx)) {
            //printf("%s\n", uc.getName().c_str());
            //printf("false\n");
            continue;
        }
        for(size_t j = 0; j < hubs.size(); ++j) {
            const string& hub = hubs[j];
            bool hubAdc = hub.compare(0, 6, "adc://") == 0 || hub.compare(0, 7, "adcs://") == 0;
            bool commandAdc = uc.getHub().compare(0, 6, "adc://") == 0 || uc.getHub().compare(0, 7, "adcs://") == 0;
            if(hubAdc && commandAdc) {
                if((uc.getHub() == "adc://" || uc.getHub() == "adcs://") ||
                        ((uc.getHub() == "adc://op" || uc.getHub() == "adcs://op") && isOp[j]) ||
                        (uc.getHub() == hub) )
                {
                    //printf("Found ADC command for ADC hub.\n");
                    lst.push_back(*i);
                    break;
                }
            } else if((!hubAdc && !commandAdc) || uc.isChat()) {
                if((uc.getHub().length() == 0) ||
                        (uc.getHub() == "op" && isOp[j]) ||
                        (uc.getHub() == hub) )
                {
                    //printf("Found non-ADC command for non-ADC hub.\n");
                    lst.push_back(*i);
                    break;
                }
            }
        }
    }
    return lst;
}



} // namespace dcpp
