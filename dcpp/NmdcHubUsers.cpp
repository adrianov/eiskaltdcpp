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
#include "StringTokenizer.h"

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

namespace {

bool isNickLike(const string& nick) {
    if(nick.empty() || nick.size() > 64)
        return false;
    for(unsigned char c : nick) {
        if(c <= 32 || c == ':' || c == '|' || c == '$' || c == '<' || c == '>')
            return false;
    }
    return true;
}

string trimCopy(string s) {
    while(!s.empty() && s.front() == ' ')
        s.erase(0, 1);
    while(!s.empty() && s.back() == ' ')
        s.pop_back();
    return s;
}

void setClientFromSegment(Identity& id, string segment) {
    string::size_type j;
    if((j = segment.find("V:")) != string::npos || (j = segment.find("v:")) != string::npos) {
        string app = trimCopy(segment.substr(0, j));
        if(!app.empty())
            id.set("AP", app);
        id.set("VE", segment.substr(j + 2));
    } else if((j = segment.find(' ')) != string::npos) {
        id.set("AP", trimCopy(segment.substr(0, j)));
        id.set("VE", trimCopy(segment.substr(j + 1)));
    } else if(segment.find("++") != string::npos) {
        id.set("AP", "++");
    } else if(segment.find(':') == string::npos && !segment.empty()) {
        id.set("AP", segment);
    }
}

} // namespace

void NmdcHub::stopInfectedConnect(const string& message, const string& aNick) {
    if((Util::findSubString(message, "зараж") == string::npos &&
        Util::findSubString(message, "infected") == string::npos) ||
       (Util::findSubString(message, "отклон") == string::npos &&
        Util::findSubString(message, "reject") == string::npos))
        return;

    string nick = aNick;
    if(nick.empty()) {
        auto pos = message.rfind(':');
        if(pos == string::npos || pos + 1 >= message.length())
            return;

        nick = message.substr(pos + 1);
        while(!nick.empty() && (nick.back() == ' ' || nick.back() == '\r'))
            nick.pop_back();
        while(!nick.empty() && nick.front() == ' ')
            nick.erase(0, 1);
        if(nick.empty())
            return;
    }

    if(!isNickLike(nick))
        return;

    OnlineUser* u = findUser(nick);
    if(u) {
        ClientManager::getInstance()->stopConnect(*u);
    } else {
        UserPtr user = ClientManager::getInstance()->getUser(nick, getHubUrl());
        ClientManager::getInstance()->stopConnect(HintedUser(user, getHubUrl()));
    }
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

void NmdcHub::updateFromTag(Identity& id, const string& tag) {
    StringTokenizer<string> tok(tag, ',');
    id.set("US", Util::emptyString);
    bool clientSet = false;
    for(auto& i: tok.getTokens()) {
        if(i.size() < 2)
            continue;

        if(i.compare(0, 2, "H:") == 0) {
            StringTokenizer<string> t(i.substr(2), '/');
            if(t.getTokens().size() != 3)
                continue;
            id.set("HN", t.getTokens()[0]);
            id.set("HR", t.getTokens()[1]);
            id.set("HO", t.getTokens()[2]);
        } else if(i.compare(0, 2, "S:") == 0) {
            id.set("SL", i.substr(2));
        } else if(i.compare(0, 2, "M:") == 0) {
            if(i.size() == 3) {
                if(i[2] == 'A')
                    id.getUser()->unsetFlag(User::PASSIVE);
                else
                    id.getUser()->setFlag(User::PASSIVE);
            }
        } else if(i.compare(0, 2, "L:") == 0) {
            id.set("US", Util::toString(Util::toInt(i.substr(2)) * 1024));
        } else if(!clientSet && i.compare(0, 2, "H:") != 0 && i.compare(0, 2, "S:") != 0 &&
                  i.compare(0, 2, "M:") != 0 && i.compare(0, 2, "L:") != 0 &&
                  i.compare(0, 2, "O:") != 0 && i.compare(0, 2, "C:") != 0) {
            setClientFromSegment(id, i);
            clientSet = true;
        }
    }
    id.set("TA", '<' + tag + '>');
}

void NmdcHub::findTagInMyINFO(Identity& id, const string& param, size_t start) {
    if(!id.get("TA").empty())
        return;

    for(size_t lt = param.find('<', start); lt != string::npos; lt = param.find('<', lt + 1)) {
        size_t gt = param.find('>', lt);
        if(gt == string::npos || gt <= lt + 1)
            break;

        const string tag = param.substr(lt + 1, gt - lt - 1);
        if(tag.find(",M:") != string::npos || tag.find(",H:") != string::npos ||
           tag.find("V:") != string::npos || tag.find("v:") != string::npos ||
           tag.find("++") != string::npos) {
            updateFromTag(id, tag);
            return;
        }
    }
}
} // namespace dcpp
