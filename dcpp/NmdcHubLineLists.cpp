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

#include "ChatMessage.h"
#include "HubSearchDenied.h"
#include "StringTokenizer.h"

namespace dcpp {

void NmdcHub::onLineUserLists(const string& cmd, const string& param) {
    if(cmd == "$UserIP") {
        if(param.empty())
            return;

        OnlineUserList v;
        StringTokenizer<string> t(param, "$$");
        for(auto& it: t.getTokens()) {
            string::size_type j = it.find(' ');
            if(j == string::npos || j + 1 >= it.length())
                continue;

            OnlineUser* u = findUser(it.substr(0, j));
            if(!u)
                continue;

            u->getIdentity().setIp(it.substr(j + 1));
            if(u->getUser() == getMyIdentity().getUser())
                setMyIdentity(u->getIdentity());
            v.push_back(u);
        }
        updated(v);
    } else if(cmd == "$NickList") {
        if(param.empty())
            return;

        OnlineUserList v;
        StringTokenizer<string> t(param, "$$");
        for(auto& it: t.getTokens()) {
            if(!it.empty())
                v.push_back(&getUser(it));
        }

        if(!(supportFlags & SUPPORTS_NOGETINFO) && !v.empty()) {
            string tmp;
            tmp.reserve(v.size() * (11 + 10 + getMyNick().length()));
            string n = ' ' + fromUtf8(getMyNick()) + '|';
            for(auto& i: v) {
                tmp += "$GetINFO ";
                tmp += fromUtf8(i->getIdentity().getNick());
                tmp += n;
            }
            send(tmp);
        }

        updated(v);
    } else if(cmd == "$OpList") {
        if(param.empty())
            return;

        OnlineUserList v;
        StringTokenizer<string> t(param, "$$");
        for(auto& it: t.getTokens()) {
            if(it.empty())
                continue;
            OnlineUser& ou = getUser(it);
            ou.getIdentity().setOp(true);
            if(ou.getUser() == getMyIdentity().getUser())
                setMyIdentity(ou.getIdentity());
            v.push_back(&ou);
        }

        updated(v);
        updateCounts(false);
        myInfo(false);
    }
}

void NmdcHub::onLineTo(const string& param) {
    string::size_type i = param.find("From:");
    if(i == string::npos)
        return;

    i += 6;
    string::size_type j = param.find('$', i);
    if(j == string::npos)
        return;

    string rtNick = param.substr(i, j - 1 - i);
    if(rtNick.empty())
        return;
    i = j + 1;

    if(param.size() < i + 3 || param[i] != '<')
        return;

    j = param.find('>', i);
    if(j == string::npos)
        return;

    string fromNick = param.substr(i + 1, j - i - 1);
    if(fromNick.empty() || param.size() < j + 2)
        return;

    ChatMessage message = { unescape(param.substr(j + 2)), findUser(fromNick), &getUser(getMyNick()), findUser(rtNick), false, 0 };

    if(!message.replyTo || !message.from) {
        if(!message.replyTo) {
            OnlineUser& ou = getUser(rtNick);
            ou.getIdentity().setHub(true);
            ou.getIdentity().setHidden(true);
            updated(ou);
        }
        if(!message.from) {
            OnlineUser& ou = getUser(fromNick);
            ou.getIdentity().setHub(true);
            ou.getIdentity().setHidden(true);
            updated(ou);
        }
        message.replyTo = findUser(rtNick);
        message.from = findUser(fromNick);
    }

    if(message.from)
        noteSearchRateLimit(searchQueue, message.text);

    fire(ClientListener::Message(), this, message);
}

} // namespace dcpp
