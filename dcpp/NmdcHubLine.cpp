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

#include "ChatMessage.h"
#include "ConnectionManager.h"
#include "CryptoManager.h"
#include "PeerConnectLog.h"
#include "PeerConnectTls.h"
#include "format.h"
#include "HubSearchDenied.h"
#include "SearchManager.h"
#include "StringTokenizer.h"
#include "UserCommand.h"

namespace dcpp {

void NmdcHub::onLine(const string& aLine) noexcept {
    if(aLine.length() == 0)
        return;

    if(aLine[0] != '$') {
        // Check if we're being banned...
        if(state != STATE_NORMAL) {
            if(Util::findSubString(aLine, "banned") != string::npos) {
                setAutoReconnect(false);
            }
        }
        string line = toUtf8(aLine);
        if(line[0] != '<') {
            stopInfectedConnect(unescape(line));
            noteSearchDenied(*this, unescape(line));
            fire(ClientListener::StatusMessage(), this, unescape(line));
            return;
        }
        string::size_type i = line.find('>', 2);
        if(i == string::npos) {
            fire(ClientListener::StatusMessage(), this, unescape(line));
            return;
        }
        string nick = line.substr(1, i-1);
        string message;
        if((line.length()-1) > i) {
            message = line.substr(i+2);
        } else {
            fire(ClientListener::StatusMessage(), this, unescape(line));
            return;
        }

        if((line.find("Hub-Security") != string::npos) && (line.find("was kicked by") != string::npos)) {
            fire(ClientListener::StatusMessage(), this, unescape(line), ClientListener::FLAG_IS_SPAM);
            return;
        } else if((line.find("is kicking") != string::npos) && (line.find("because:") != string::npos)) {
            fire(ClientListener::StatusMessage(), this, unescape(line), ClientListener::FLAG_IS_SPAM);
            return;
        }

        stopInfectedConnect(unescape(message), nick);
        noteSearchDenied(*this, unescape(message));

        ChatMessage chatMessage = { unescape(message), findUser(nick), nullptr, nullptr, false, 0 };

        if(!chatMessage.from) {
            OnlineUser& o = getUser(nick);
            // Assume that messages from unknown users come from the hub
            o.getIdentity().setHub(true);
            o.getIdentity().setHidden(true);
            updated(o);
            chatMessage.from = &o;
        }

        fire(ClientListener::Message(), this, chatMessage);
        return;
    }

    string cmd;
    string param;
    string::size_type x;

    if( (x = aLine.find(' ')) == string::npos) {
        cmd = aLine;
    } else {
        cmd = aLine.substr(0, x);
        param = toUtf8(aLine.substr(x+1));
    }

    if(cmd == "$Search") {
        if(state != STATE_NORMAL) {
            return;
        }
        string::size_type i = 0;
        string::size_type j = param.find(' ', i);
        if(j == string::npos || i == j)
            return;

        string seeker = param.substr(i, j-i);
        auto pos_slashes = seeker.find("://");
        if (pos_slashes != string::npos)
            return;

        // Filter own searches
        if(isActive()) {
            if(seeker == (getLocalIp() + ":" + SearchManager::getInstance()->getPort())) {
                return;
            }
        } else {
            // Hub:seeker
            if(seeker.size() > 4 &&
                    Util::stricmp(seeker.c_str() + 4, getMyNick().c_str()) == 0) {
                return;
            }
        }

        i = j + 1;

        uint64_t tick = GET_TICK();
        clearFlooders(tick);

        seekers.emplace_back(seeker, tick);

        // First, check if it's a flooder
        for(auto& fi: flooders) {
            if(fi.first == seeker) {
                return;
            }
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
        if(param[i] == 'F') {
            a = SearchManager::SIZE_DONTCARE;
        } else if(param[i+2] == 'F') {
            a = SearchManager::SIZE_ATLEAST;
        } else {
            a = SearchManager::SIZE_ATMOST;
        }
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

        // without terms, this is an invalid search.
        if(!terms.empty()) {
            if(seeker.compare(0, 4, "Hub:") == 0) {
                OnlineUser* u = findUser(seeker.substr(4));

                if(u == NULL) {
                    return;
                }

                if(!u->getUser()->isSet(User::PASSIVE)) {
                    u->getUser()->setFlag(User::PASSIVE);
                    updated(*u);
                }
            }

            fire(ClientListener::NmdcSearch(), this, seeker, a, Util::toInt64(size), type, terms);
        }
    } else if(cmd == "$MyINFO") {
        string::size_type i, j;
        i = 5;
        j = param.find(' ', i);
        if( (j == string::npos) || (j == i) )
            return;
        string nick = param.substr(i, j-i);

        if(nick.empty())
            return;

        i = j + 1;

        OnlineUser& u = getUser(nick);

        // If he is already considered to be the hub (thus hidden), probably should appear in the UserList
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
        // Look for a tag...
        if(!tmpDesc.empty() && tmpDesc.back() == '>') {
            x = tmpDesc.rfind('<');
            if(x != string::npos) {
                // Hm, we have something...disassemble it...
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
            // No connection = bot...
            u.getUser()->setFlag(User::BOT);
            u.getIdentity().setHub(false);
        } else {
            u.getUser()->unsetFlag(User::BOT);
            u.getIdentity().setBot(false);
        }

        u.getIdentity().setHub(false);

        u.getIdentity().setConnection(connection);
        u.getIdentity().setStatus(Util::toString(param[j-1]));

        if(u.getIdentity().getStatus() & Identity::TLS) {
            u.getUser()->setFlag(User::TLS);
        } else {
            u.getUser()->unsetFlag(User::TLS);
        }

        if(u.getIdentity().getStatus() & Identity::NAT) {
            u.getUser()->setFlag(User::NAT_TRAVERSAL);
        } else {
            u.getUser()->unsetFlag(User::NAT_TRAVERSAL);
        }
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

        if(u.getUser() == getMyIdentity().getUser()) {
            setMyIdentity(u.getIdentity());
        }

        updated(u);
    } else if(cmd == "$Quit") {
        if(!param.empty()) {
            const string& nick = param;
            OnlineUser* u = findUser(nick);
            if(!u)
                return;

            fire(ClientListener::UserRemoved(), this, *u);

            putUser(nick);
        }
    } else if(cmd == "$ConnectToMe") {
        if(state != STATE_NORMAL) {
            return;
        }
        string::size_type i = param.find(' ');
        string::size_type j;
        if( (i == string::npos) || ((i + 1) >= param.size()) ) {
            return;
        }
        i++;
        j = param.find(':', i);
        if(j == string::npos) {
            return;
        }
        string server = param.substr(i, j-i);
        if(j+1 >= param.size()) {
            return;
        }
        string senderNick;
        string port;

        i = param.find(' ', j+1);
        if(i == string::npos) {
            port = param.substr(j+1);
        } else {
            senderNick = param.substr(i+1);
            port = param.substr(j+1, i-j-1);
        }

        bool secure = false;
        if(port[port.size() - 1] == 'S') {
            port.erase(port.size() - 1);
            if(CryptoManager::getInstance()->TLSOk()) {
                secure = true;
            }
        }

        if(BOOLSETTING(ALLOW_NATT)) {
            if(port[port.size() - 1] == 'N') {
                if(senderNick.empty())
                    return;

                port.erase(port.size() - 1);

                // Trigger connection attempt sequence locally ...
                ConnectionManager::getInstance()->nmdcConnect(server, port, sock->getLocalPort(),
                                                              BufferedSocket::NAT_CLIENT, getMyNick(), getHubUrl(), getEncoding(), secure);

                // ... and signal other client to do likewise.
                send("$ConnectToMe " + fromUtf8(senderNick) + " " + getLocalIp() + ":" + sock->getLocalPort() + (secure ? "RS" : "R") + "|");
                return;
            } else if(port[port.size() - 1] == 'R') {
                port.erase(port.size() - 1);

                // Trigger connection attempt sequence locally
                ConnectionManager::getInstance()->nmdcConnect(server, port, sock->getLocalPort(),
                                                              BufferedSocket::NAT_SERVER, getMyNick(), getHubUrl(), getEncoding(), secure);
                return;
            }
        }

        if(port.empty()) {
            PeerConnectLog::nmdcRecv(getHubUrl(), str(F_("$ConnectToMe ignored: empty port")));
            return;
        }
        PeerConnectLog::nmdcRecv(getHubUrl(), str(F_("$ConnectToMe from %1%:%2%") % server % port));
        // For simplicity, we make the assumption that users on a hub have the same character encoding
        ConnectionManager::getInstance()->nmdcConnect(server, port, getMyNick(), getHubUrl(), getEncoding(), secure);
    } else if(cmd == "$RevConnectToMe") {
        if(state != STATE_NORMAL) {
            return;
        }

        string::size_type j = param.find(' ');
        if(j == string::npos) {
            return;
        }

        OnlineUser* u = findUser(param.substr(0, j));
        if(u == NULL) {
            PeerConnectLog::nmdcRecv(getHubUrl(), str(F_("$RevConnectToMe: sender %1% not in user list") % param.substr(0, j)));
            return;
        }

        if(isActive()) {
            PeerConnectLog::nmdcRecv(*u, "$RevConnectToMe, replying with $ConnectToMe");
            connectToMe(*u);
        } else if(BOOLSETTING(ALLOW_NATT) && (u->getIdentity().getStatus() & Identity::NAT)) {
            PeerConnectLog::nmdcRecv(*u, "$RevConnectToMe, NAT traversal");
            bool secure = PeerConnectTls::resolveSecure(PeerConnectTls::AUTO, u->getUser());
            // NMDC v2.205 supports "$ConnectToMe sender_nick remote_nick ip:port", but many NMDC hubsofts block it
            // sender_nick at the end should work at least in most used hubsofts
            send("$ConnectToMe " + fromUtf8(u->getIdentity().getNick()) + " " +
                 getLocalIp() + ":" + sock->getLocalPort() +
                 (secure ? "NS " : "N ") + fromUtf8(getMyNick()) + "|");
        } else {
            if(!u->getUser()->isSet(User::PASSIVE)) {
                PeerConnectLog::nmdcRecv(*u, "$RevConnectToMe, both passive — asking them to connect");
                u->getUser()->setFlag(User::PASSIVE);
                // Notify the user that we're passive too...
                revConnectToMe(*u);
                updated(*u);

                return;
            }
            PeerConnectLog::nmdcRecv(*u, "$RevConnectToMe, both passive — no connection possible");
        }
    } else if(cmd == "$SR") {
        SearchManager::getInstance()->onSearchResult(aLine);
    } else if(cmd == "$HubName") {
        // If " - " found, the first part goes to hub name, rest to description
        // If no " - " found, first word goes to hub name, rest to description

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
        StringList& sl = st.getTokens();
        for(auto& i: sl) {
            if(i == "UserCommand") {
                supportFlags |= SUPPORTS_USERCOMMAND;
            } else if(i == "NoGetINFO") {
                supportFlags |= SUPPORTS_NOGETINFO;
            } else if(i == "UserIP2") {
                supportFlags |= SUPPORTS_USERIP2;
            }
        }
    } else if(cmd == "$UserCommand") {
        string::size_type i = 0;
        string::size_type j = param.find(' ');
        if(j == string::npos)
            return;

        int type = Util::toInt(param.substr(0, j));
        i = j+1;
        if(type == UserCommand::TYPE_SEPARATOR || type == UserCommand::TYPE_CLEAR) {
            int ctx = Util::toInt(param.substr(i));
            fire(ClientListener::HubUserCommand(), this, type, ctx, Util::emptyString, Util::emptyString);
        } else if(type == UserCommand::TYPE_RAW || type == UserCommand::TYPE_RAW_ONCE) {
            j = param.find(' ', i);
            if(j == string::npos)
                return;
            int ctx = Util::toInt(param.substr(i));
            i = j+1;
            j = param.find('$');
            if(j == string::npos)
                return;
            string name = unescape(param.substr(i, j-i));
            // NMDC uses '\' as a separator but both ADC and our internal representation use '/'
            Util::replace("/", "//", name);
            Util::replace("\\", "/", name);
            i = j+1;
            string command = unescape(param.substr(i, param.length() - i));
            fire(ClientListener::HubUserCommand(), this, type, ctx, name, command);
        }
    } else if(cmd == "$Lock") {
        if(state != STATE_PROTOCOL) {
            return;
        }
        state = STATE_IDENTIFY;

        // Param must not be toUtf8'd...
        param = aLine.substr(6);

        if(!param.empty()) {
            string::size_type j = param.find(" Pk=");
            string lock, pk;
            if( j != string::npos ) {
                lock = param.substr(0, j);
                pk = param.substr(j + 4);
            } else {
                // Workaround for faulty linux hubs...
                j = param.find(" ");
                if(j != string::npos)
                    lock = param.substr(0, j);
                else
                    lock = param;
            }

            if(CryptoManager::getInstance()->isExtended(lock)) {
                StringList feat = {
                    "UserCommand",
                    "NoGetINFO",
                    "NoHello",
                    "UserIP2",
                    "TTHSearch",
                    "ZPipe0"
                };

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
        }
    } else if(cmd == "$Hello") {
        if(!param.empty()) {
            OnlineUser& u = getUser(param);

            if(u.getUser() == getMyIdentity().getUser()) {
                if(isActive())
                    u.getUser()->unsetFlag(User::PASSIVE);
                else
                    u.getUser()->setFlag(User::PASSIVE);
            }

            if(state == STATE_IDENTIFY && u.getUser() == getMyIdentity().getUser()) {
                state = STATE_NORMAL;
                storeHubNick();
                updateCounts(false);

                version();
                getNickList();
                myInfo(true);
            }

            updated(u);
        }
    } else if(cmd == "$ForceMove") {
        disconnect(false);
        fire(ClientListener::Redirect(), this, param);
    } else if(cmd == "$HubIsFull") {
        fire(ClientListener::HubFull(), this);
    } else if(cmd == "$HubTopic") {
        //dcdebug("Nmdc topic:%s",aLine.c_str());
        string line;
        string str2= _("Hub topic:");
        line=toUtf8(aLine);
        line.replace(0,9,str2);
        fire(ClientListener::StatusMessage(), this, unescape(line), ClientListener::FLAG_NORMAL);
    } else if(cmd == "$ValidateDenide") {       // Mind the spelling...
        if(tryAlternateNick())
            return;
        disconnect(false);
        fire(ClientListener::NickTaken(), this);
    } else if(cmd == "$UserIP") {
        if(!param.empty()) {
            OnlineUserList v;
            StringTokenizer<string> t(param, "$$");
            StringList& l = t.getTokens();
            for(auto& it: l) {
                string::size_type j = 0;
                if((j = it.find(' ')) == string::npos)
                    continue;
                if((j+1) == it.length())
                    continue;

                OnlineUser* u = findUser(it.substr(0, j));

                if(!u)
                    continue;

                u->getIdentity().setIp(it.substr(j+1));
                if(u->getUser() == getMyIdentity().getUser()) {
                    setMyIdentity(u->getIdentity());
                }
                v.push_back(u);
            }

            updated(v);
        }
    } else if(cmd == "$NickList") {
        if(!param.empty()) {
            OnlineUserList v;
            StringTokenizer<string> t(param, "$$");
            StringList& sl = t.getTokens();

            for(auto& it: sl) {
                if(it.empty())
                    continue;

                v.push_back(&getUser(it));
            }

            if(!(supportFlags & SUPPORTS_NOGETINFO)) {
                string tmp;
                // Let's assume 10 characters per nick...
                tmp.reserve(v.size() * (11 + 10 + getMyNick().length()));
                string n = ' ' + fromUtf8(getMyNick()) + '|';
                for(auto& i: v) {
                    tmp += "$GetINFO ";
                    tmp += fromUtf8(i->getIdentity().getNick());
                    tmp += n;
                }
                if(!tmp.empty()) {
                    send(tmp);
                }
            }

            updated(v);
        }
    } else if(cmd == "$OpList") {
        if(!param.empty()) {
            OnlineUserList v;
            StringTokenizer<string> t(param, "$$");
            StringList& sl = t.getTokens();
            for(auto& it: sl) {
                if(it.empty())
                    continue;
                OnlineUser& ou = getUser(it);
                ou.getIdentity().setOp(true);
                if(ou.getUser() == getMyIdentity().getUser()) {
                    setMyIdentity(ou.getIdentity());
                }
                v.push_back(&ou);
            }

            updated(v);
            updateCounts(false);

            // Special...to avoid op's complaining that their count is not correctly
            // updated when they log in (they'll be counted as registered first...)
            myInfo(false);
        }
    } else if(cmd == "$To:") {
        string::size_type i = param.find("From:");
        if(i == string::npos)
            return;

        i+=6;
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

        string fromNick = param.substr(i+1, j-i-1);
        if(fromNick.empty())
            return;

        if(param.size() < j + 2) {
            return;
        }

        ChatMessage message = { unescape(param.substr(j + 2)), findUser(fromNick), &getUser(getMyNick()), findUser(rtNick), false, 0 };

        if(!message.replyTo || !message.from) {
            if(!message.replyTo) {
                // Assume it's from the hub
                OnlineUser& ou = getUser(rtNick);
                ou.getIdentity().setHub(true);
                ou.getIdentity().setHidden(true);
                updated(ou);
            }
            if(!message.from) {
                // Assume it's from the hub
                OnlineUser& ou = getUser(fromNick);
                ou.getIdentity().setHub(true);
                ou.getIdentity().setHidden(true);
                updated(ou);
            }

            // Update pointers just in case they've been invalidated
            message.replyTo = findUser(rtNick);
            message.from = findUser(fromNick);
        }

        fire(ClientListener::Message(), this, message);
    } else if(cmd == "$GetPass") {
        OnlineUser& ou = getUser(getMyNick());
        ou.getIdentity().set("RG", "1");
        setMyIdentity(ou.getIdentity());
        fire(ClientListener::GetPassword(), this);
    } else if(cmd == "$BadPass") {
        setPassword(Util::emptyString);
    } else if(cmd == "$ZOn") {
        try {
            sock->setMode(BufferedSocket::MODE_ZPIPE);
        } catch (const Exception& e) {
            dcdebug("NmdcHub::onLine %s failed with error: %s\n", cmd.c_str(), e.getError().c_str());
        }
    } else {
        dcassert(cmd[0] == '$');
        dcdebug("NmdcHub::onLine Unknown command %s\n", aLine.c_str());
    }
}
} // namespace dcpp
