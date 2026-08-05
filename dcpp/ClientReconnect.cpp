/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hub reconnect backoff and nick-fallback reconnect helpers.
 */

#include "stdinc.h"
#include "Client.h"

#include "BufferedSocket.h"
#include "FavoriteManager.h"
#include "format.h"
#include "HubReconnectFilter.h"
#include "Util.h"

namespace dcpp {

void Client::reconnect() {
    HubReconnectFilter::clearToday(getHubUrl());
    setReconnAttempts(0);
    setAutoReconnect(true);
    setReconnDelay(0);
    urgentReconnect = true;
    FavoriteManager::getInstance()->removeUserCommand(getHubUrl());

    // Drop the current socket on the UI thread (removeListener first so an
    // in-flight Failed cannot schedule "Reconnect planned"), then connect now.
    state = STATE_DISCONNECTED;
    if(sock) {
        sock->removeListener(this);
        BufferedSocket::putSocket(sock);
        sock = 0;
    }
    connect();
}

void Client::scheduleReconnectBackoff() {
    const int attempts = HubReconnectFilter::noteDisconnect(getHubUrl());
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

    if(urgentReconnect) {
        // Manual Reconnect: never publish "Reconnect planned in 1 minute".
        // Stay urgent until Connected; bound retries via today count.
        setAutoReconnect(true);
        setReconnDelay(0);
        const int today = HubReconnectFilter::noteDisconnect(getHubUrl());
        setReconnAttempts(today);
        if(HubReconnectFilter::shouldGiveUp(today)) {
            urgentReconnect = false;
            setAutoReconnect(false);
            fire(ClientListener::StatusMessage(), this,
                 str(F_("Giving up hub reconnect after %1% failed attempts today") % today),
                 ClientListener::FLAG_NORMAL);
        }
    } else if(getAutoReconnect() && HubReconnectFilter::shouldGiveUp(HubReconnectFilter::todayCount(getHubUrl()))) {
        setAutoReconnect(false);
        fire(ClientListener::StatusMessage(), this,
             str(F_("Giving up hub reconnect after %1% failed attempts today") % HubReconnectFilter::todayCount(getHubUrl())),
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

    // Socket thread: do not putSocket/connect here. Mark DISCONNECTED with
    // delay 0 so Second retries immediately (Failed is suppressed on
    // graceless disconnect).
    state = STATE_DISCONNECTED;
    if(sock) {
        sock->removeListener(this);
        disconnect(true);
    }
    setAutoReconnect(true);
    HubReconnectFilter::clearToday(getHubUrl());
    setReconnAttempts(0);
    setReconnDelay(0);
    updateActivity();
    return true;
}

void Client::storeHubNick() {
    FavoriteManager::getInstance()->setHubNick(getHubUrl(), getCurrentNick());
}

} // namespace dcpp
