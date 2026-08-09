/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
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

#include "../NmdcHub.h"
#include "ChatLine.h"

#include "../BufferedSocket.h"
#include "../SearchManager.h"
#include "../format.h"

namespace dcpp {

void NmdcHub::onLine(const string& aLine) noexcept {
    if(aLine.empty())
        return;

    // Flylink hubs sometimes prepend null/control bytes before a real command.
    if(static_cast<unsigned char>(aLine[0]) < 32) {
        size_t start = 1;
        while(start < aLine.size() && static_cast<unsigned char>(aLine[start]) < 32)
            ++start;
        if(start < aLine.size())
            onLine(aLine.substr(start));
        return;
    }

    if(aLine[0] != '$') {
        NmdcChatLine(*this).handle(toUtf8(aLine));
        return;
    }

    string cmd;
    string param;
    string::size_type x = aLine.find(' ');
    if(x == string::npos)
        cmd = aLine;
    else {
        cmd = aLine.substr(0, x);
        param = toUtf8(aLine.substr(x + 1));
    }

    if(cmd == "$Search")
        onLineSearch(param);
    else if(cmd == "$MyINFO")
        onLineMyInfo(param);
    else if(cmd == "$Quit") {
        if(!param.empty()) {
            OnlineUser* u = findUser(param);
            if(u) {
                fire(ClientListener::UserRemoved(), this, *u);
                putUser(param);
            }
        }
    } else if(cmd == "$ConnectToMe")
        onConnectToMe(param);
    else if(cmd == "$RevConnectToMe")
        onRevConnectToMe(param);
    else if(cmd == "$SR")
        SearchManager::getInstance()->onSearchResult(aLine);
    else if(cmd == "$HubName" || cmd == "$Supports" || cmd == "$UserCommand" || cmd == "$Lock" || cmd == "$Hello")
        onLineHubSetup(cmd, param, aLine);
    else if(cmd == "$ForceMove") {
        disconnect(false);
        if(!handleRedirect(param))
            fire(ClientListener::Redirect(), this, param);
    } else if(cmd == "$HubIsFull")
        fire(ClientListener::HubFull(), this);
    else if(cmd == "$ValidateDenide") {
        if(tryAlternateNick())
            return;
        disconnect(false);
        fire(ClientListener::NickTaken(), this);
    } else if(cmd == "$HubTopic" || cmd == "$GetHubURL" || cmd == "$SearchRule" ||
              cmd == "$NickRule" || cmd == "$BadNick")
        onLineHubExt(cmd, param);
    else if(cmd == "$UserIP" || cmd == "$NickList" || cmd == "$OpList" || cmd == "$BotList")
        onLineUserLists(cmd, param);
    else if(cmd == "$To:")
        onLineTo(param);
    else if(cmd == "$LogedIn")
        fire(ClientListener::StatusMessage(), this,
             str(F_("You are an operator on %1%") % getHubUrl()));
    else if(cmd == "$GetPass") {
        OnlineUser& ou = getUser(getMyNick());
        ou.getIdentity().set("RG", "1");
        setMyIdentity(ou.getIdentity());
        fire(ClientListener::GetPassword(), this);
    } else if(cmd == "$BadPass")
        setPassword(Util::emptyString);
    else if(cmd == "$ZOn") {
        try {
            sock->setMode(BufferedSocket::MODE_ZPIPE);
        } catch(const Exception& e) {
            dcdebug("NmdcHub::onLine %s failed with error: %s\n", cmd.c_str(), e.getError().c_str());
        }
    } else {
        dcassert(cmd[0] == '$');
        dcdebug("NmdcHub::onLine Unknown command %s\n", aLine.c_str());
    }
}

} // namespace dcpp
