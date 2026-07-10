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

#include "BufferedSocket.h"
#include "ChatMessage.h"
#include "HubSearchDenied.h"
#include "SearchManager.h"
#include "format.h"

namespace dcpp {

void NmdcHub::onLine(const string& aLine) noexcept {
    if(aLine.empty())
        return;

    if(aLine[0] != '$') {
        if(state != STATE_NORMAL && Util::findSubString(aLine, "banned") != string::npos)
            setAutoReconnect(false);

        string line = toUtf8(aLine);
        if(line[0] != '<') {
            const string text = unescape(line);
            stopInfectedConnect(text);
            noteSecureCtmRejected(text);
            noteSearchDenied(*this, text);
            fire(ClientListener::StatusMessage(), this, text);
            return;
        }

        string::size_type i = line.find('>', 2);
        if(i == string::npos) {
            fire(ClientListener::StatusMessage(), this, unescape(line));
            return;
        }

        string nick = line.substr(1, i - 1);
        if((line.length() - 1) <= i) {
            fire(ClientListener::StatusMessage(), this, unescape(line));
            return;
        }

        string message = line.substr(i + 2);
        if((line.find("Hub-Security") != string::npos) && (line.find("was kicked by") != string::npos)) {
            fire(ClientListener::StatusMessage(), this, unescape(line), ClientListener::FLAG_IS_SPAM);
            return;
        }
        if((line.find("is kicking") != string::npos) && (line.find("because:") != string::npos)) {
            fire(ClientListener::StatusMessage(), this, unescape(line), ClientListener::FLAG_IS_SPAM);
            return;
        }

        string body = unescape(message);
        stopInfectedConnect(body, nick);
        noteSecureCtmRejected(body);

        ChatMessage chatMessage = { body, findUser(nick), nullptr, nullptr, false, 0 };
        if(!chatMessage.from) {
            OnlineUser& o = getUser(nick);
            o.getIdentity().setHub(true);
            o.getIdentity().setHidden(true);
            updated(o);
            chatMessage.from = &o;
        }

        const Identity& id = chatMessage.from->getIdentity();
        if(id.isHub() || id.isBot() || id.isOp() || chatMessage.from->getUser()->isSet(User::BOT))
            noteSearchDenied(*this, body);

        fire(ClientListener::Message(), this, chatMessage);
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
    else if(cmd == "$HubTopic") {
        string line = toUtf8(aLine);
        line.replace(0, 9, _("Hub topic:"));
        fire(ClientListener::StatusMessage(), this, unescape(line), ClientListener::FLAG_NORMAL);
    } else if(cmd == "$ValidateDenide") {
        if(tryAlternateNick())
            return;
        disconnect(false);
        fire(ClientListener::NickTaken(), this);
    } else if(cmd == "$UserIP" || cmd == "$NickList" || cmd == "$OpList")
        onLineUserLists(cmd, param);
    else if(cmd == "$To:")
        onLineTo(param);
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
