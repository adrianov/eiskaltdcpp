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
#include "SimpleXML.h"
#include "Util.h"
#include "format.h"

namespace dcpp {

void FavoriteManager::load(SimpleXML& aXml) {
    dontSave = true;
    bool needSave = false;

    aXml.resetCurrentChild();
    if(aXml.findChild("Hubs")) {
        aXml.stepIn();

        while(aXml.findChild("Group")) {
            string name = aXml.getChildAttrib("Name");
            if(name.empty())
                continue;
            FavHubGroupProperties props = { aXml.getBoolChildAttrib("Private"), aXml.getBoolChildAttrib("Connect") };
            favHubGroups[name] = props;
        }

        aXml.resetCurrentChild();
        while(aXml.findChild("Hub")) {
            FavoriteHubEntry* e = new FavoriteHubEntry();
            e->setName(aXml.getChildAttrib("Name"));
            e->setConnect(aXml.getBoolChildAttrib("Connect"));
            e->setHubDescription(aXml.getChildAttrib("Description"));
            e->setNick(aXml.getChildAttrib("Nick"));
            e->setPassword(aXml.getChildAttrib("Password"));
            e->setServer(aXml.getChildAttrib("Server"));
            e->setUserDescription(aXml.getChildAttrib("UserDescription"));
            e->setEncoding(aXml.getChildAttrib("Encoding"));
            e->setExternalIP(aXml.getChildAttrib("ExternalIP"));
            e->setClientId(aXml.getChildAttrib("ClientId"));
            e->setOverrideId(Util::toInt(aXml.getChildAttrib("OverrideId")) != 0);
            e->setUseInternetIP(aXml.getBoolChildAttrib("UseInternetIp"));
            e->setDisableChat(aXml.getBoolChildAttrib("DisableChat"));
            e->setMode(Util::toInt(aXml.getChildAttrib("Mode")));
            e->setSearchInterval(Util::toInt(aXml.getChildAttrib("SearchInterval")));
            e->setGroup(aXml.getChildAttrib("Group"));
            favoriteHubs.push_back(e);

            if(aXml.getBoolChildAttrib("Connect")) {
                // this entry dates from before the window manager & fav hub groups; convert it.
                const string name = _("Auto-connect group (converted)");
                if(favHubGroups.find(name) == favHubGroups.end()) {
                    FavHubGroupProperties props = { false, true };
                    favHubGroups[name] = props;
                }
                e->setGroup(name);
                needSave = true;
            }
        }
        try {
            aXml.stepOut();
        }
        catch(const Exception&) { }
    }
    // parse groups that have the "Connect" param and send their hubs to WindowManager
    //for(auto i = favHubGroups.begin(), iend = favHubGroups.end(); i != iend; ++i) {
    //if(i->second.connect) {
    //FavoriteHubEntryList hubs = getFavoriteHubs(i->first);
    //for(auto hub = hubs.begin(), hub_end = hubs.end(); hub != hub_end; ++hub) {
    //StringMap map;
    //map[WindowInfo::address] = (*hub)->getServer();
    //WindowManager::getInstance()->add(WindowManager::hub(), map);
    //}
    //}
    //}

    aXml.resetCurrentChild();
    if(aXml.findChild("Users")) {
        aXml.stepIn();
        while(aXml.findChild("User")) {
            UserPtr u;
            const string& cid = aXml.getChildAttrib("CID");
            const string& nick = aXml.getChildAttrib("Nick");
            const string& hubUrl = aXml.getChildAttrib("URL");

            if(cid.length() != 39) {
                if(nick.empty() || hubUrl.empty())
                    continue;
                u = ClientManager::getInstance()->getUser(nick, hubUrl);
            } else {
                u = ClientManager::getInstance()->getUser(CID(cid));
            }
            auto i = users.emplace(u->getCID(), FavoriteUser(u, nick, hubUrl)).first;

            if(aXml.getBoolChildAttrib("GrantSlot"))
                i->second.setFlag(FavoriteUser::FLAG_GRANTSLOT);

            i->second.setLastSeen((uint32_t)aXml.getIntChildAttrib("LastSeen"));
            i->second.setDescription(aXml.getChildAttrib("UserDescription"));

        }
        try {
            aXml.stepOut();
        }
        catch(const Exception&) { }
    }
    aXml.resetCurrentChild();
    if(aXml.findChild("UserCommands")) {
        aXml.stepIn();
        while(aXml.findChild("UserCommand")) {
            addUserCommand(aXml.getIntChildAttrib("Type"), aXml.getIntChildAttrib("Context"), 0, aXml.getChildAttrib("Name"),
                           aXml.getChildAttrib("Command"), aXml.getChildAttrib("To"), aXml.getChildAttrib("Hub"));
        }
        try {
            aXml.stepOut();
        }
        catch(const Exception&) { }
    }
    //Favorite download to dirs
    aXml.resetCurrentChild();
    if(aXml.findChild("FavoriteDirs")) {
        aXml.stepIn();
        while(aXml.findChild("Directory")) {
            string virt = aXml.getChildAttrib("Name");
            string d(aXml.getChildData());
            FavoriteManager::getInstance()->addFavoriteDir(d, virt);
        }
        try {
            aXml.stepOut();
        }
        catch(const Exception&) { }
    }

    dontSave = false;
    if(needSave)
        save();
}

} // namespace dcpp
