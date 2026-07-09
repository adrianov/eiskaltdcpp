/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
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
#include "Client.h"

#include "BufferedSocket.h"
#include "ClientManager.h"
#include "DebugManager.h"
#include "FavoriteManager.h"
#include "format.h"
#include "HubReconnectFilter.h"
#include "TimerManager.h"
#include "Util.h"
#include "Text.h"

namespace dcpp {

Client::Counts Client::counts;

uint32_t idCounter = 0;

Client::Client(const string& hubURL, char separator_, bool secure_, Socket::Protocol proto_) :
    myIdentity(ClientManager::getInstance()->getMe(), 0), uniqueId(++idCounter),
    reconnDelay(120), reconnAttempts(0), lastActivity(GET_TICK()), registered(false), autoReconnect(false), searchBlocked(false),
    encoding(Text::hubDefaultCharset), state(STATE_DISCONNECTED), sock(0),
    hubUrl(hubURL), separator(separator_), proto(proto_),
    secure(secure_), countType(COUNT_UNCOUNTED)
{
    string file, proto, query, fragment;
    Util::decodeUrl(hubURL, proto, address, port, file, query, fragment);

    keyprint = Util::decodeQuery(query)["kp"];

    TimerManager::getInstance()->addListener(this);
}

Client::~Client() {
    dcassert(!sock);

    FavoriteManager::getInstance()->removeUserCommand(getHubUrl());
    TimerManager::getInstance()->removeListener(this);
    updateCounts(true);
}

void Client::reconnect() {
    disconnect(true);
    setReconnAttempts(0);
    setAutoReconnect(true);
    setReconnDelay(0);
}

void Client::resetReconnBackoff() {
    setReconnAttempts(0);
    setReconnDelay(HubReconnectFilter::delaySec(1));
}

void Client::scheduleReconnectBackoff() {
    const uint32_t attempts = getReconnAttempts() + 1;
    setReconnAttempts(attempts);
    const int delay = HubReconnectFilter::delaySec(attempts);
    setReconnDelay(delay);
    const time_t nextAt = GET_TIME() + delay;
    const string timeStr = delay >= 3600 ? Util::getTimeString(nextAt, "%Y-%m-%d %H:%M") : Util::getTimeString(nextAt);
    fire(ClientListener::StatusMessage(), this,
         str(F_("Reconnect planned at %1% (in %2%)") % timeStr % HubReconnectFilter::delayLabel(attempts)),
         ClientListener::FLAG_NORMAL);
}

void Client::onConnectFailed(const string& aLine) {
    state = STATE_DISCONNECTED;
    FavoriteManager::getInstance()->removeUserCommand(getHubUrl());
    if(sock)
        sock->removeListener(this);
    if(getAutoReconnect() && HubReconnectFilter::shouldGiveUp(getReconnAttempts())) {
        setAutoReconnect(false);
        fire(ClientListener::StatusMessage(), this,
             str(F_("Giving up hub reconnect after %1% failed attempts") % getReconnAttempts()),
             ClientListener::FLAG_NORMAL);
    } else if(getAutoReconnect()) {
        scheduleReconnectBackoff();
    }
    updateActivity();
    fire(ClientListener::Failed(), this, aLine);
}

void Client::on(Failed, const string& aLine) noexcept {
    onConnectFailed(aLine);
}

void Client::shutdown() {
    if(sock) {
        BufferedSocket::putSocket(sock);
        sock = 0;
    }
}

bool Client::isActive() const {
    return ClientManager::getInstance()->isActive(hubUrl);
}

void Client::updateCounts(bool aRemove) {
    if(countType == COUNT_NORMAL) {
        counts.normal.dec();
    } else if(countType == COUNT_REGISTERED) {
        counts.registered.dec();
    } else if(countType == COUNT_OP) {
        counts.op.dec();
    }

    countType = COUNT_UNCOUNTED;

    if(!aRemove) {
        if(getMyIdentity().isOp()) {
            counts.op.inc();
            countType = COUNT_OP;
        } else if(getMyIdentity().isRegistered()) {
            counts.registered.inc();
            countType = COUNT_REGISTERED;
        } else {
            counts.normal.inc();
            countType = COUNT_NORMAL;
        }
    }
}

void Client::updated(OnlineUser& user) {
    fire(ClientListener::UserUpdated(), this, user);
}

void Client::updated(OnlineUserList& users) {
    fire(ClientListener::UsersUpdated(), this, users);
}

void Client::on(Line, const string& aLine) noexcept {
    updateActivity();
    COMMAND_DEBUG((Util::stricmp(getEncoding(), Text::utf8) != 0 ? Text::toUtf8(aLine, getEncoding()) : aLine), DebugManager::HUB_IN, getIpPort())
}

bool Client::tryAlternateNick() {
    FavoriteManager* fm = FavoriteManager::getInstance();
    const string oldNick = getCurrentNick();
    const string next = fm->nextHubNick(getHubUrl(), oldNick);
    if(next.empty())
        return false;

    fm->setHubNick(getHubUrl(), next);
    setCurrentNick(checkNick(next));

    fire(ClientListener::StatusMessage(), this,
         str(F_("Nick \"%1%\" is taken, trying \"%2%\"...") % oldNick % next),
         ClientListener::FLAG_NORMAL);

    disconnect(true);
    setAutoReconnect(true);
    resetReconnBackoff();
    return true;
}

void Client::storeHubNick() {
    FavoriteManager::getInstance()->setHubNick(getHubUrl(), getCurrentNick());
}

#ifdef LUA_SCRIPT
string ClientScriptInstance::formatChatMessage(const tstring& aLine) {
    Lock l(cs);
    string processed = Text::fromT(aLine);
    MakeCall("dcpp", "FormatChatText", 1, (Client*)this, processed);

    if (lua_isstring(L, -1)) processed = lua_tostring(L, -1);

    lua_settop(L, 0);
    return Text::toT(processed);
}

bool ClientScriptInstance::onHubFrameEnter(Client* aClient, const string& aLine) {
    Lock l(cs);
    MakeCall("dcpp", "OnCommandEnter", 1, aClient, aLine);
    return GetLuaBool();
}
#endif
} // namespace dcpp
