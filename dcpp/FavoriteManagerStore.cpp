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
#include "File.h"
#include "SimpleXML.h"
#include "Util.h"
#include "format.h"

namespace dcpp {

void FavoriteManager::save() {
    if(dontSave)
        return;

    Lock l(cs);
    try {
        SimpleXML xml;

        xml.addTag("Favorites");
        xml.stepIn();

        xml.addTag("Hubs");
        xml.stepIn();

        for(auto i = favHubGroups.begin(), iend = favHubGroups.end(); i != iend; ++i) {
            xml.addTag("Group");
            xml.addChildAttrib("Name", i->first);
            xml.addChildAttrib("Private", i->second.priv);
            xml.addChildAttrib("Connect", i->second.connect);
        }

        for(auto i = favoriteHubs.begin(), iend = favoriteHubs.end(); i != iend; ++i) {
            xml.addTag("Hub");
            xml.addChildAttrib("Name", (*i)->getName());
            xml.addChildAttrib("Connect", (*i)->getConnect());
            xml.addChildAttrib("Description", (*i)->getHubDescription());
            xml.addChildAttrib("Nick", (*i)->getNick(false));
            xml.addChildAttrib("Password", (*i)->getPassword());
            xml.addChildAttrib("Server", (*i)->getServer());
            xml.addChildAttrib("UserDescription", (*i)->getUserDescription());
            xml.addChildAttrib("Encoding", (*i)->getEncoding());
            xml.addChildAttrib("ClientId", (*i)->getClientId());
            xml.addChildAttrib("ExternalIP", (*i)->getExternalIP());
            xml.addChildAttrib("OverrideId", Util::toString((*i)->getOverrideId()));
            xml.addChildAttrib("UseInternetIp",(*i)->getUseInternetIP());
            xml.addChildAttrib("DisableChat", (*i)->getDisableChat());
            xml.addChildAttrib("Mode", Util::toString((*i)->getMode()));
            xml.addChildAttrib("SearchInterval", Util::toString((*i)->getSearchInterval()));
            xml.addChildAttrib("Group", (*i)->getGroup());
        }
        xml.stepOut();
        xml.addTag("Users");
        xml.stepIn();
        for(auto i = users.begin(), iend = users.end(); i != iend; ++i) {
            xml.addTag("User");
            xml.addChildAttrib("LastSeen", i->second.getLastSeen());
            xml.addChildAttrib("GrantSlot", i->second.isSet(FavoriteUser::FLAG_GRANTSLOT));
            xml.addChildAttrib("UserDescription", i->second.getDescription());
            xml.addChildAttrib("Nick", i->second.getNick());
            xml.addChildAttrib("URL", i->second.getUrl());
            xml.addChildAttrib("CID", i->first.toBase32());
        }
        xml.stepOut();
        xml.addTag("UserCommands");
        xml.stepIn();
        for(auto i = userCommands.begin(), iend = userCommands.end(); i != iend; ++i) {
            if(!i->isSet(UserCommand::FLAG_NOSAVE)) {
                xml.addTag("UserCommand");
                xml.addChildAttrib("Type", i->getType());
                xml.addChildAttrib("Context", i->getCtx());
                xml.addChildAttrib("Name", i->getName());
                xml.addChildAttrib("Command", i->getCommand());
                xml.addChildAttrib("To", i->getTo());
                xml.addChildAttrib("Hub", i->getHub());
            }
        }
        xml.stepOut();
        //Favorite download to dirs
        xml.addTag("FavoriteDirs");
        xml.stepIn();
        StringPairList spl = getFavoriteDirs();
        for(auto i = spl.begin(), iend = spl.end(); i != iend; ++i) {
            xml.addTag("Directory", i->first);
            xml.addChildAttrib("Name", i->second);
        }
        xml.stepOut();

        xml.stepOut();

        string fname = getConfigFile();

        File f(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
        f.write(SimpleXML::utf8Header);
        f.write(xml.toXML());
        f.close();
        File::deleteFile(fname);
        File::renameFile(fname + ".tmp", fname);

    } catch(const Exception& e) {
        dcdebug("FavoriteManager::save: %s\n", e.getError().c_str());
    }
}

void FavoriteManager::load() {

    // Add NMDC standard op commands
    static const char kickstr[] =
            "$To: %[userNI] From: %[myNI] $<%[myNI]> You are being kicked because: %[line:Reason]|<%[myNI]> is kicking %[userNI] because: %[line:Reason]|$Kick %[userNI]|";
    addUserCommand(UserCommand::TYPE_RAW_ONCE, UserCommand::CONTEXT_USER | UserCommand::CONTEXT_SEARCH, UserCommand::FLAG_NOSAVE,
                   _("Kick user(s)"), kickstr, "", "op");
    static const char redirstr[] =
            "$OpForceMove $Who:%[userNI]$Where:%[line:Target Server]$Msg:%[line:Message]|";
    addUserCommand(UserCommand::TYPE_RAW_ONCE, UserCommand::CONTEXT_USER | UserCommand::CONTEXT_SEARCH, UserCommand::FLAG_NOSAVE,
                   _("Redirect user(s)"), redirstr, "", "op");

    try {
        SimpleXML xml;
        Util::migrate(getConfigFile());
        xml.fromXML(File(getConfigFile(), File::READ, File::OPEN).read());

        if(xml.findChild("Favorites")) {
            xml.stepIn();
            load(xml);
            xml.stepOut();
        }
    } catch(const Exception& e) {
        dcdebug("FavoriteManager::load: %s\n", e.getError().c_str());
    }
}

} // namespace dcpp
