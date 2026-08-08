/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
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

namespace {

void fireReconnectAt(Client& c, int delaySec, const string& delayLabel) {
    c.setReconnDelay(delaySec);
    const time_t nextAt = GET_TIME() + delaySec;
    const string timeStr = delaySec >= 3600 ? Util::getTimeString(nextAt, "%Y-%m-%d %H:%M")
                                            : Util::getTimeString(nextAt);
    c.fire(ClientListener::StatusMessage(), &c,
           str(F_("Reconnecting at %1% (in %2%)") % timeStr % delayLabel),
           ClientListener::FLAG_NORMAL);
}

void fireGiveUp(Client& c, int today) {
    c.setAutoReconnect(false);
    c.fire(ClientListener::StatusMessage(), &c,
           str(F_("Giving up after %1% failed reconnects today") % today),
           ClientListener::FLAG_NORMAL);
}

} // namespace

void Client::reconnect() {
    HubReconnectFilter::clearToday(getHubUrl());
    setReconnAttempts(0);
    setAutoReconnect(true);
    urgentReconnect = true;
    FavoriteManager::getInstance()->removeUserCommand(getHubUrl());
    updateCounts(true);

    if(sock) {
        sock->removeListener(this);
        BufferedSocket::putSocket(sock);
        sock = 0;
    }
    // Hub Failed clears OnlineUsers and schedules the 5s wait (works even when
    // the socket listener was already removed after an earlier fail).
    // Empty line: user-requested leave — no "Fail: …" status.
    on(Failed(), Util::emptyString);
    // Leaving on purpose must not consume a daily reconnect attempt.
    HubReconnectFilter::clearToday(getHubUrl());
    setReconnAttempts(0);
}

void Client::scheduleReconnectBackoff() {
    const int attempts = HubReconnectFilter::noteDisconnect(getHubUrl());
    setReconnAttempts(attempts);
    fireReconnectAt(*this, HubReconnectFilter::delaySec(attempts),
                    HubReconnectFilter::delayLabel(attempts));
}

void Client::onConnectFailed(const string& aLine) {
    state = STATE_DISCONNECTED;
    FavoriteManager::getInstance()->removeUserCommand(getHubUrl());
    if(sock)
        sock->removeListener(this);

    // Manual Reconnect: fixed 5s wait (no exponential backoff) until login/ban/give-up.
    if(urgentReconnect) {
        const int today = HubReconnectFilter::noteDisconnect(getHubUrl());
        setReconnAttempts(today);
        if(!getAutoReconnect() || HubReconnectFilter::shouldGiveUp(today)) {
            urgentReconnect = false;
            setAutoReconnect(false);
            if(HubReconnectFilter::shouldGiveUp(today))
                fireGiveUp(*this, today);
        } else {
            fireReconnectAt(*this, HubReconnectFilter::MANUAL_DELAY_SEC,
                            HubReconnectFilter::manualDelayLabel());
        }
    } else if(getAutoReconnect()) {
        const int today = HubReconnectFilter::todayCount(getHubUrl());
        if(HubReconnectFilter::shouldGiveUp(today))
            fireGiveUp(*this, today);
        else
            scheduleReconnectBackoff();
    }
    updateActivity();
    // Always notify UI to clear users / refresh; empty aLine means user leave
    // (no "Fail:" text) — see HubFrame / gtk Hub Failed handlers.
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
         str(F_("Nick \"%1%\" is taken; switching to \"%2%\"...") % oldNick % next),
         ClientListener::FLAG_NORMAL);

    // Leave quietly, then retry on the next Second tick with the new nick.
    if(sock)
        disconnect(true);
    setAutoReconnect(true);
    HubReconnectFilter::clearToday(getHubUrl());
    setReconnAttempts(0);
    setReconnDelay(0);
    updateActivity();
    return true;
}

void Client::storeHubNick() {
    FavoriteManager::getInstance()->setHubNick(getHubUrl(), getCurrentNick());
    urgentReconnect = false;
    setReconnAttempts(0);
}

} // namespace dcpp
