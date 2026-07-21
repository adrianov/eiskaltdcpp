/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "ConnectionManager.h"

#include "ClientManager.h"
#include "PeerConnectHub.h"
#include "PeerConnectLog.h"
#include "PeerConnectFilter.h"
#include "PeerConnectTls.h"
#include "QueueManager.h"
#include "UserConnection.h"

namespace dcpp {

namespace {

bool isPostHandshakeClose(UserConnection::States s) {
    return s == UserConnection::STATE_SND || s == UserConnection::STATE_IDLE ||
           s == UserConnection::STATE_GET || s == UserConnection::STATE_SEND;
}

} // namespace

void ConnectionManager::markQueueGiveUp(ConnectionQueueItem* cqi, int attempts, bool slotWait) {
    if(slotWait)
        PeerConnectLog::queueSlotWaitGiveUp(cqi->getUser(), attempts);
    else
        PeerConnectLog::queueGiveUp(cqi->getUser(), attempts);
    cqi->setErrors(-1);
    cqi->setLastAttempt(GET_TICK());
}

bool ConnectionManager::dropUnreachableDownload(ConnectionQueueItem* cqi) {
    if(!cqi || PeerConnectHub::wasPeerReached(cqi->getUser().user))
        return false;
    if(!ClientManager::getInstance()->allHubsConnectTimedOut(cqi->getUser().user))
        return false;
    PeerConnectLog::queueUnreachable(cqi->getUser());
    PeerConnectHub::clearPeerSession(cqi->getUser().user);
    return true;
}

bool ConnectionManager::onDownloadConnectTimeout(ConnectionQueueItem* cqi) {
    cqi->setErrors(cqi->getErrors() + 1);
    // Zero lastAttempt: full CONNECTING wait already elapsed — hub-rotate now.
    cqi->setLastAttempt(0);
    const string timedOutHub = cqi->getUser().hint;
    PeerConnectHub::noteConnectTimeout(cqi->getUser().user, timedOutHub);
    PeerConnectHub::rememberFailure(cqi->getUser().user, timedOutHub);
    // Drop hint so the next resolve does not soft-boost this hub.
    cqi->setHubHint(Util::emptyString);
    clearOutgoingConnect(cqi->getUser().user);

    if(dropUnreachableDownload(cqi))
        return true;

    if(PeerConnectFilter::shouldGiveUp(cqi->getErrors())) {
        markQueueGiveUp(cqi, cqi->getErrors(), false);
    } else {
        PeerConnectLog::queueTimeout(cqi->getUser(), cqi->getErrors());
        fire(ConnectionManagerListener::Failed(), cqi, _("Connection timeout"));
    }
    cqi->setState(ConnectionQueueItem::WAITING);
    return false;
}

void ConnectionManager::reviveDownloadQueue(ConnectionQueueItem* cqi, bool forced) {
    if(!cqi || cqi->getErrors() != -1)
        return;
    if(!forced && cqi->getLastAttempt() != 0 &&
            GET_TICK() < cqi->getLastAttempt() + static_cast<uint64_t>(PeerConnectFilter::GIVE_UP_COOLDOWN_MS))
        return;
    cqi->setErrors(0);
    cqi->setSlotWaits(0);
    cqi->setLastAttempt(0);
    cqi->setConnectAttempts(0);
    clearOutgoingConnect(cqi->getUser().user);
}

void ConnectionManager::failDownloadQueue(ConnectionQueueItem* dlCqi, UserConnection* aSource, const string& aError, bool protocolError) {
    // Nothing left to fetch (e.g. file list just finished): drop instead of
    // slot-wait / Failed UI that would stick as "Connection closed".
    if(QueueManager::getInstance()->hasDownload(dlCqi->getUser()) == QueueItem::PAUSED) {
        putCQI(dlCqi);
        return;
    }

    // A download socket existed — peer answered (IP and/or inbound connect).
    PeerConnectHub::notePeerReached(dlCqi->getUser().user);

    const bool tlsMismatch = protocolError && PeerConnectTls::isTlsMismatch(aError);
    const bool postClose = isPostHandshakeClose(aSource->getState()) && !protocolError;
    const bool hadSlot = dlCqi->getGrantedSlot();
    // Peer already uploaded at least one file on this socket, then dropped while
    // idle or while we asked for the next file — reconnect for the rest without
    // counting toward give-up. Latch (if still armed) paces quick fail loops.
    const bool softReconnect = hadSlot && !protocolError &&
            (aSource->getState() == UserConnection::STATE_IDLE ||
             aSource->getState() == UserConnection::STATE_SND);
    const bool slotWait = postClose && !protocolError && !hadSlot;
    dlCqi->setGrantedSlot(false);

    if(!tlsMismatch)
        PeerConnectTls::scheduleRetry(dlCqi, aSource->isSecure(), protocolError, aSource->getState(), aError);

    if(slotWait) {
        dlCqi->setSlotWaits(dlCqi->getSlotWaits() + 1);
        dlCqi->setLastAttempt(GET_TICK());
        const int backoffMs = PeerConnectFilter::slotWaitBackoffMs(dlCqi->getSlotWaits());
        PeerConnectLog::queueSlotWait(dlCqi->getUser(), dlCqi->getSlotWaits(), backoffMs / (60 * 1000));
        if(PeerConnectFilter::shouldGiveUpSlotWait(dlCqi->getSlotWaits()))
            markQueueGiveUp(dlCqi, dlCqi->getSlotWaits(), true);
    } else if(softReconnect) {
        dlCqi->setLastAttempt(0);
    } else if(tlsMismatch) {
        PeerConnectTls::scheduleRetry(dlCqi, aSource->isSecure(), protocolError, aSource->getState(), aError);
        dlCqi->setErrors(dlCqi->getErrors() + 1);
        dlCqi->setLastAttempt(GET_TICK());
        if(PeerConnectFilter::shouldGiveUp(dlCqi->getErrors()))
            markQueueGiveUp(dlCqi, dlCqi->getErrors(), false);
    } else if(protocolError) {
        dlCqi->setErrors(-1);
        dlCqi->setLastAttempt(GET_TICK());
    } else {
        dlCqi->setErrors(dlCqi->getErrors() + 1);
        dlCqi->setLastAttempt(GET_TICK());
        if(PeerConnectFilter::shouldGiveUp(dlCqi->getErrors()))
            markQueueGiveUp(dlCqi, dlCqi->getErrors(), false);
    }

    dlCqi->setState(ConnectionQueueItem::WAITING);
    if(!slotWait && !softReconnect) {
        const string hub = (aSource && !aSource->getHubUrl().empty()) ? aSource->getHubUrl()
                : dlCqi->getUser().hint;
        PeerConnectHub::rememberFailure(dlCqi->getUser().user, hub);
    }
    fire(ConnectionManagerListener::Failed(), dlCqi, aError);
}

void ConnectionManager::failed(UserConnection* aSource, const string& aError, bool protocolError) {
    Lock l(cs);

    ConnectionQueueItem::Ptr dlCqi = nullptr;
    if(aSource->getUser()) {
        auto i = find(downloads.begin(), downloads.end(), aSource->getUser());
        dlCqi = i != downloads.end() ? *i : findDownloadCqi(aSource->getHintedUser());
    }

    const bool tlsMismatch = protocolError && PeerConnectTls::isTlsMismatch(aError);
    const bool slotWait = isPostHandshakeClose(aSource->getState()) && !protocolError;

    if(dlCqi && aSource->getUser() && !slotWait) {
        if(!protocolError || (tlsMismatch && PeerConnectFilter::shouldLogTimeout(dlCqi->getErrors() + 1)))
            PeerConnectLog::connectionFail(aSource, aError, protocolError);
    }

    if(aSource->isSet(UserConnection::FLAG_ASSOCIATED)) {
        if(aSource->isSet(UserConnection::FLAG_DOWNLOAD) && dlCqi) {
            failDownloadQueue(dlCqi, aSource, aError, protocolError);
        } else if(aSource->isSet(UserConnection::FLAG_UPLOAD)) {
            auto i = find(uploads.begin(), uploads.end(), aSource->getUser());
            if(i != uploads.end())
                putCQI(*i);
        }
    } else if(dlCqi && aSource->isSet(UserConnection::FLAG_DOWNLOAD)) {
        failDownloadQueue(dlCqi, aSource, aError, protocolError);
    }
    putConnection(aSource);
}

} // namespace dcpp
