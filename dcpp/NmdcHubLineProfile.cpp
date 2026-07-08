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

#include "PeerConnectTls.h"
#include "SearchManager.h"
#include "format.h"

namespace dcpp {

void NmdcHub::onLineSearch(const string& param) {
    if(state != STATE_NORMAL)
        return;

    string::size_type i = 0;
    string::size_type j = param.find(' ', i);
    if(j == string::npos || i == j)
        return;

    string seeker = param.substr(i, j-i);
    if(seeker.find("://") != string::npos)
        return;

    if(isActive()) {
        if(seeker == (getLocalIp() + ":" + SearchManager::getInstance()->getPort()))
            return;
    } else if(seeker.size() > 4 && Util::stricmp(seeker.c_str() + 4, getMyNick().c_str()) == 0) {
        return;
    }

    i = j + 1;

    uint64_t tick = GET_TICK();
    clearFlooders(tick);
    seekers.emplace_back(seeker, tick);

    for(auto& fi: flooders) {
        if(fi.first == seeker)
            return;
    }

    int count = 0;
    for(auto& fi: seekers) {
        if(fi.first == seeker)
            count++;
        if(count > 7) {
            if(seeker.compare(0, 4, "Hub:") == 0)
                fire(ClientListener::SearchFlood(), this, seeker.substr(4));
            else
                fire(ClientListener::SearchFlood(), this, str(F_("%1% (Nick unknown)") % seeker));
            flooders.emplace_back(seeker, tick);
            return;
        }
    }

    int a;
    if(param[i] == 'F')
        a = SearchManager::SIZE_DONTCARE;
    else if(param[i+2] == 'F')
        a = SearchManager::SIZE_ATLEAST;
    else
        a = SearchManager::SIZE_ATMOST;

    i += 4;
    j = param.find('?', i);
    if(j == string::npos || i == j)
        return;
    string size = param.substr(i, j-i);
    i = j + 1;
    j = param.find('?', i);
    if(j == string::npos || i == j)
        return;
    int type = Util::toInt(param.substr(i, j-i)) - 1;
    i = j + 1;
    string terms = unescape(param.substr(i));

    if(terms.empty())
        return;

    if(seeker.compare(0, 4, "Hub:") == 0) {
        OnlineUser* u = findUser(seeker.substr(4));
        if(!u)
            return;

        if(!u->getUser()->isSet(User::PASSIVE)) {
            u->getUser()->setFlag(User::PASSIVE);
            updated(*u);
        }

        if(!(supportFlags & SUPPORTS_NOGETINFO)) {
            const Identity& id = u->getIdentity();
            if(id.getApplication().empty() && id.getConnection().empty() &&
               id.getBytesShared() <= 0 && !id.isSet("GI")) {
                u->getIdentity().set("GI", "1");
                send("$GetINFO " + fromUtf8(id.getNick()) + ' ' + fromUtf8(getMyNick()) + '|');
            }
        }
    }

    fire(ClientListener::NmdcSearch(), this, seeker, a, Util::toInt64(size), type, terms);
}

void NmdcHub::onLineMyInfo(const string& param) {
    string::size_type i = 5;
    string::size_type j = param.find(' ', i);
    if(j == string::npos || j == i)
        return;

    string nick = param.substr(i, j-i);
    if(nick.empty())
        return;

    i = j + 1;
    OnlineUser& u = getUser(nick);

    if(u.getIdentity().isHidden()) {
        u.getIdentity().setHidden(false);
        u.getIdentity().setHub(false);
    }

    j = param.find('$', i);
    if(j == string::npos)
        return;

    string tmpDesc = unescape(param.substr(i, j-i));
    while(!tmpDesc.empty() && tmpDesc.back() == ' ')
        tmpDesc.pop_back();

    if(!tmpDesc.empty() && tmpDesc.back() == '>') {
        string::size_type x = tmpDesc.rfind('<');
        if(x != string::npos) {
            updateFromTag(u.getIdentity(), tmpDesc.substr(x + 1, tmpDesc.length() - x - 2));
            tmpDesc.erase(x);
        }
    }
    u.getIdentity().setDescription(tmpDesc);
    findTagInMyINFO(u.getIdentity(), param, i);

    i = j + 3;
    j = param.find('$', i);
    if(j == string::npos)
        return;

    string connection = param.substr(i, j-i-1);
    if(connection.empty()) {
        u.getUser()->setFlag(User::BOT);
        u.getIdentity().setHub(false);
    } else {
        u.getUser()->unsetFlag(User::BOT);
        u.getIdentity().setBot(false);
    }

    u.getIdentity().setHub(false);
    u.getIdentity().setConnection(connection);
    u.getIdentity().setStatus(Util::toString(param[j-1]));

    if(u.getIdentity().getStatus() & Identity::TLS)
        u.getUser()->setFlag(User::TLS);
    else if(!PeerConnectTls::learnedTlsRequired(u.getUser()))
        u.getUser()->unsetFlag(User::TLS);

    if(u.getIdentity().getStatus() & Identity::NAT)
        u.getUser()->setFlag(User::NAT_TRAVERSAL);
    else
        u.getUser()->unsetFlag(User::NAT_TRAVERSAL);

    i = j + 1;
    j = param.find('$', i);
    if(j == string::npos)
        return;

    u.getIdentity().setEmail(unescape(param.substr(i, j-i)));

    i = j + 1;
    j = param.find('$', i);
    if(j == string::npos)
        return;
    u.getIdentity().setBytesShared(param.substr(i, j-i));

    if(u.getUser() == getMyIdentity().getUser())
        setMyIdentity(u.getIdentity());

    updated(u);
}

} // namespace dcpp
