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

#include "CryptoManager.h"
#include "StringTokenizer.h"
#include "UserCommand.h"

namespace dcpp {

void NmdcHub::onLineHubSetup(const string& cmd, const string& param, const string& aLine) {
    if(cmd == "$HubName") {
        string::size_type i = param.find(" - ");
        if(i == string::npos) {
            i = param.find(' ');
            if(i == string::npos) {
                getHubIdentity().setNick(unescape(param));
                getHubIdentity().setDescription(Util::emptyString);
            } else {
                getHubIdentity().setNick(unescape(param.substr(0, i)));
                getHubIdentity().setDescription(unescape(param.substr(i+1)));
            }
        } else {
            getHubIdentity().setNick(unescape(param.substr(0, i)));
            getHubIdentity().setDescription(unescape(param.substr(i+3)));
        }
        fire(ClientListener::HubUpdated(), this);
    } else if(cmd == "$Supports") {
        StringTokenizer<string> st(param, ' ');
        for(auto& i: st.getTokens()) {
            if(i == "UserCommand")
                supportFlags |= SUPPORTS_USERCOMMAND;
            else if(i == "NoGetINFO")
                supportFlags |= SUPPORTS_NOGETINFO;
            else if(i == "UserIP2")
                supportFlags |= SUPPORTS_USERIP2;
        }
    } else if(cmd == "$UserCommand") {
        string::size_type i = 0;
        string::size_type j = param.find(' ');
        if(j == string::npos)
            return;

        int type = Util::toInt(param.substr(0, j));
        i = j + 1;
        if(type == UserCommand::TYPE_SEPARATOR || type == UserCommand::TYPE_CLEAR) {
            int ctx = Util::toInt(param.substr(i));
            fire(ClientListener::HubUserCommand(), this, type, ctx, Util::emptyString, Util::emptyString);
        } else if(type == UserCommand::TYPE_RAW || type == UserCommand::TYPE_RAW_ONCE) {
            j = param.find(' ', i);
            if(j == string::npos)
                return;
            int ctx = Util::toInt(param.substr(i));
            i = j + 1;
            j = param.find('$');
            if(j == string::npos)
                return;
            string name = unescape(param.substr(i, j-i));
            Util::replace("/", "//", name);
            Util::replace("\\", "/", name);
            i = j + 1;
            string command = unescape(param.substr(i, param.length() - i));
            fire(ClientListener::HubUserCommand(), this, type, ctx, name, command);
        }
    } else if(cmd == "$Lock") {
        if(state != STATE_PROTOCOL)
            return;
        state = STATE_IDENTIFY;

        string lockParam = aLine.substr(6);
        if(lockParam.empty())
            return;

        string::size_type j = lockParam.find(" Pk=");
        string lock;
        if(j != string::npos)
            lock = lockParam.substr(0, j);
        else if((j = lockParam.find(' ')) != string::npos)
            lock = lockParam.substr(0, j);
        else
            lock = lockParam;

        if(CryptoManager::getInstance()->isExtended(lock)) {
            StringList feat = { "UserCommand", "NoGetINFO", "NoHello", "UserIP2", "TTHSearch", "ZPipe0" };
            if(CryptoManager::getInstance()->TLSOk())
                feat.push_back("TLS");
#ifdef WITH_DHT
            if(BOOLSETTING(USE_DHT))
                feat.push_back("DHT0");
#endif
            supports(feat);
        }

        key(CryptoManager::getInstance()->makeKey(lock));
        OnlineUser& ou = getUser(getCurrentNick());
        validateNick(ou.getIdentity().getNick());
    } else if(cmd == "$Hello") {
        if(param.empty())
            return;

        OnlineUser& u = getUser(param);
        if(u.getUser() == getMyIdentity().getUser()) {
            if(isActive())
                u.getUser()->unsetFlag(User::PASSIVE);
            else
                u.getUser()->setFlag(User::PASSIVE);
        }

        if(state == STATE_IDENTIFY && u.getUser() == getMyIdentity().getUser()) {
            state = STATE_NORMAL;
            resetReconnBackoff();
            storeHubNick();
            updateCounts(false);
            version();
            getNickList();
            myInfo(true);
        }

        updated(u);
    }
}

} // namespace dcpp
