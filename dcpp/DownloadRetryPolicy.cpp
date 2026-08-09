/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "DownloadRetryPolicy.h"

#include "ClientManager.h"
#include "ConnectionQueueItem.h"
#include "PeerConnectFilter.h"
#include "PeerConnectHub.h"
#include "PeerConnectLog.h"
#include "PeerConnectTls.h"
#include "UserConnection.h"

namespace dcpp {

namespace {

bool isPostHandshakeClose(UserConnection::States s) {
    return s == UserConnection::STATE_SND || s == UserConnection::STATE_IDLE ||
           s == UserConnection::STATE_GET || s == UserConnection::STATE_SEND;
}

/** Give up only when the peer answered at least once; otherwise leave errors at
 *  the threshold so the timer can still drop the peer as unreachable. */
void giveUpIfReached(ConnectionQueueItem* cqi) {
    if(!PeerConnectFilter::shouldGiveUp(cqi->getErrors()))
        return;
    if(PeerConnectHub::wasPeerReached(cqi->getUser().user))
        DownloadRetryPolicy::markGiveUp(cqi, cqi->getErrors(), false);
}

} // namespace

DownloadRetryPolicy::DownloadRetryPolicy(const UserConnection* aSource, const string& aError,
        bool aProtocolError) noexcept :
    source(aSource),
    error(aError),
    protocolError(aProtocolError),
    tlsMismatch(aProtocolError && PeerConnectTls::isTlsMismatch(aError)),
    postClose(!aProtocolError && aSource && isPostHandshakeClose(aSource->getState()))
{
}

void DownloadRetryPolicy::logFail(int errors) const {
    if(postClose)
        return;
    if(!protocolError || (tlsMismatch && PeerConnectFilter::shouldLogTimeout(errors + 1)))
        PeerConnectLog::connectionFail(source, error, protocolError);
}

void DownloadRetryPolicy::apply(ConnectionQueueItem* cqi) const {
    const bool hadSlot = cqi->getGrantedSlot();
    // Peer already uploaded at least one file on this socket, then dropped while
    // idle or while we asked for the next file — reconnect for the rest without
    // counting toward give-up. The CTM latch paces quick fail loops.
    const bool softReconnect = hadSlot && !protocolError &&
            (source->getState() == UserConnection::STATE_IDLE ||
             source->getState() == UserConnection::STATE_SND);
    const bool slotWait = postClose && !hadSlot;
    cqi->setGrantedSlot(false);
    if(!slotWait)
        cqi->setQueuePos(-1);

    PeerConnectTls::scheduleRetry(cqi, source->isSecure(), protocolError, source->getState(), error);

    if(slotWait) {
        cqi->setSlotWaits(cqi->getSlotWaits() + 1);
        cqi->setLastAttempt(GET_TICK());
        const int backoffMs = PeerConnectFilter::slotWaitBackoffMs(cqi->getSlotWaits());
        PeerConnectLog::queueSlotWait(cqi->getUser(), cqi->getSlotWaits(), backoffMs / (60 * 1000));
        if(PeerConnectFilter::shouldGiveUpSlotWait(cqi->getSlotWaits()))
            markGiveUp(cqi, cqi->getSlotWaits(), true);
    } else if(softReconnect) {
        cqi->setLastAttempt(0);
    } else if(protocolError && !tlsMismatch) {
        cqi->setErrors(-1);
        cqi->setLastAttempt(GET_TICK());
    } else {
        cqi->setErrors(cqi->getErrors() + 1);
        cqi->setLastAttempt(GET_TICK());
        giveUpIfReached(cqi);
    }

    cqi->setState(ConnectionQueueItem::WAITING);
    if(!slotWait && !softReconnect) {
        const string hub = source->getHubUrl().empty() ? cqi->getUser().hint : source->getHubUrl();
        PeerConnectHub::rememberFailure(cqi->getUser().user, hub);
    }
}

void DownloadRetryPolicy::markGiveUp(ConnectionQueueItem* cqi, int attempts, bool slotWait) {
    if(slotWait)
        PeerConnectLog::queueSlotWaitGiveUp(cqi->getUser(), attempts);
    else
        PeerConnectLog::queueGiveUp(cqi->getUser(), attempts);
    cqi->setErrors(-1);
    cqi->setLastAttempt(GET_TICK());
}

bool DownloadRetryPolicy::dropUnreachable(ConnectionQueueItem* cqi) {
    if(!cqi || PeerConnectHub::wasPeerReached(cqi->getUser().user))
        return false;
    // All hubs timed out, or connect give-up with no peer response (slot-wait / download).
    if(!ClientManager::getInstance()->allHubsConnectTimedOut(cqi->getUser().user) &&
            !PeerConnectFilter::shouldGiveUp(cqi->getErrors()))
        return false;
    PeerConnectLog::queueUnreachable(cqi->getUser());
    // Before putCQI / async ShareIndex jobs — block re-attach immediately.
    PeerConnectHub::noteUnreachablePeer(cqi->getUser().user);
    return true;
}

} // namespace dcpp
