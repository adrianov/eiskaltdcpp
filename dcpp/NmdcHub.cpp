/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "stdinc.h"

#include "NmdcHub.h"

#include "ClientManager.h"
#include "Socket.h"

namespace dcpp {

NmdcHub::NmdcHub(const string& aHubURL, bool secure) :
    Client(aHubURL, '|', secure, Socket::PROTO_NMDC),
    supportFlags(0),
    lastUpdate(0)
{
}

NmdcHub::~NmdcHub() {
    clearUsers();
}


#define checkstate() if(state != STATE_NORMAL) return

void NmdcHub::connect(const OnlineUser& aUser, const string&) {
    checkstate();
    dcdebug("NmdcHub::connect %s\n", aUser.getIdentity().getNick().c_str());
    if(isActive()) {
        connectToMe(aUser);
    } else {
        revConnectToMe(aUser);
    }
}

int64_t NmdcHub::getAvailable() const {
    Lock l(cs);
    int64_t x = 0;
    for(auto& i: users) {
        x+=i.second->getIdentity().getBytesShared();
    }
    return x;
}

void NmdcHub::supports(const StringList& feat) {
    string x;
    for(auto& i: feat) {
        x += i + ' ';
    }
    send("$Supports " + x + '|');
}

void NmdcHub::clearFlooders(uint64_t aTick) {
    while(!seekers.empty() && seekers.front().second + (5 * 1000) < aTick) {
        seekers.pop_front();
    }

    while(!flooders.empty() && flooders.front().second + (120 * 1000) < aTick) {
        flooders.pop_front();
    }
}

void NmdcHub::on(Connected) noexcept {
    Client::on(Connected());

    if(state != STATE_PROTOCOL) {
        return;
    }
    supportFlags = 0;
    lastMyInfoA.clear();
    lastMyInfoB.clear();
    lastMyInfoC.clear();
    lastMyInfoD.clear();
    lastUpdate = 0;
}

void NmdcHub::on(Line, const string& aLine) noexcept {
#ifdef LUA_SCRIPT
    if (onClientMessage(this, validateMessage(aLine, true)))
        return;
#endif
    if (BOOLSETTING(NMDC_DEBUG))
        fire(ClientListener::StatusMessage(), this, "<NMDC>" + aLine + "</NMDC>");
    Client::on(Line(), aLine);
    onLine(aLine);
}

void NmdcHub::on(Failed, const string& aLine) noexcept {
    clearUsers();
    Client::on(Failed(), aLine);
}

void NmdcHub::on(Second, uint64_t aTick) noexcept {
    Client::on(Second(), aTick);

    if(state == STATE_NORMAL && (aTick > (getLastActivity() + 120*1000)) ) {
        send("|", 1);
    }
}

#ifdef LUA_SCRIPT
bool NmdcHubScriptInstance::onClientMessage(NmdcHub* aClient, const string& aLine) {
    Lock l(cs);
    MakeCall("nmdch", "DataArrival", 1, aClient, aLine);
    return GetLuaBool();
}
#endif
} // namespace dcpp
