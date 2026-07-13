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
#include "Encoder.h"
#include "LogManager.h"
#include "PeerConnectLog.h"
#include "PeerConnectFilter.h"
#include "PeerConnectTls.h"
#include "QueueManager.h"
#include "UserConnection.h"
#include "format.h"

namespace dcpp {

void ConnectionManager::rejectConnection(UserConnection* aConn, const string& reason) {
    PeerConnectLog::connectionReject(aConn, reason);
    putConnection(aConn);
}

bool ConnectionManager::checkKeyprint(UserConnection *aSource) {
    auto kp = aSource->getKeyprint();
    if(kp.empty()) {
        return true;
    }

    auto kp2 = ClientManager::getInstance()->getField(aSource->getUser()->getCID(), aSource->getHubUrl(), "KP");
    if(kp2.empty()) {
        return true;
    }

    if(kp2.compare(0, 7, "SHA256/") != 0) {
        return true;
    }

    dcdebug("Keyprint: %s vs %s\n", Encoder::toBase32(&kp[0], kp.size()).c_str(), kp2.c_str() + 7);

    vector<uint8_t> kp2v(kp.size());
    Encoder::fromBase32(&kp2[7], &kp2v[0], kp2v.size());
    if(!std::equal(kp.begin(), kp.end(), kp2v.begin())) {
        dcdebug("Not equal...\n");
        return false;
    }

    return true;
}

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
}

void ConnectionManager::reviveDownloadQueue(ConnectionQueueItem* cqi) {
    if(!cqi || cqi->getErrors() != -1)
        return;
    cqi->setErrors(0);
    cqi->setSlotWaits(0);
    cqi->setLastAttempt(0);
    cqi->setConnectAttempts(0);
    clearConnectCooldown(cqi->getUser().user);
}

void ConnectionManager::failDownloadQueue(ConnectionQueueItem* dlCqi, UserConnection* aSource, const string& aError, bool protocolError) {
    // Nothing left to fetch (e.g. file list just finished): drop instead of
    // slot-wait / Failed UI that would stick as "Connection closed".
    if(QueueManager::getInstance()->hasDownload(dlCqi->getUser()) == QueueItem::PAUSED) {
        putCQI(dlCqi);
        return;
    }

    const bool tlsMismatch = protocolError && PeerConnectTls::isTlsMismatch(aError);
    const bool slotWait = isPostHandshakeClose(aSource->getState()) && !protocolError;

    if(!tlsMismatch)
        PeerConnectTls::scheduleRetry(dlCqi, aSource->isSecure(), protocolError, aSource->getState(), aError);

    if(slotWait) {
        dlCqi->setSlotWaits(dlCqi->getSlotWaits() + 1);
        dlCqi->setLastAttempt(GET_TICK());
        const int backoffMs = PeerConnectFilter::slotWaitBackoffMs(dlCqi->getSlotWaits());
        noteConnectCooldown(dlCqi->getUser().user, backoffMs);
        PeerConnectLog::queueSlotWait(dlCqi->getUser(), dlCqi->getSlotWaits(), backoffMs / (60 * 1000));
        if(PeerConnectFilter::shouldGiveUpSlotWait(dlCqi->getSlotWaits()))
            markQueueGiveUp(dlCqi, dlCqi->getSlotWaits(), true);
    } else if(tlsMismatch) {
        PeerConnectTls::scheduleRetry(dlCqi, aSource->isSecure(), protocolError, aSource->getState(), aError);
        dlCqi->setErrors(dlCqi->getErrors() + 1);
        dlCqi->setLastAttempt(GET_TICK());
        noteConnectCooldown(dlCqi->getUser().user, PeerConnectFilter::connectBackoffMs(dlCqi->getErrors()));
        if(PeerConnectFilter::shouldGiveUp(dlCqi->getErrors()))
            markQueueGiveUp(dlCqi, dlCqi->getErrors(), false);
    } else {
        dlCqi->setErrors(protocolError ? -1 : (dlCqi->getErrors() + 1));
        dlCqi->setLastAttempt(GET_TICK());
        if(!protocolError) {
            noteConnectCooldown(dlCqi->getUser().user, PeerConnectFilter::connectBackoffMs(dlCqi->getErrors()));
            if(PeerConnectFilter::shouldGiveUp(dlCqi->getErrors()))
                markQueueGiveUp(dlCqi, dlCqi->getErrors(), false);
        }
    }

    dlCqi->setState(ConnectionQueueItem::WAITING);
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

bool ConnectionManager::checkHubCCBlock(const string& aServer, const string& aPort, const string& aHubUrl)
{
    const auto server_lower = Text::toLower(aServer);
    dcassert(server_lower == aServer);

    bool cc_blocked = false;

    {
        Lock l(cs);
        cc_blocked = !hubsBlockingCC.empty() && hubsBlockingCC.find(server_lower) != hubsBlockingCC.end();
    }

    if(cc_blocked)
    {
        PeerConnectLog::skip(aServer, aHubUrl, str(F_("blocked hub IP (C-C protection) %1%:%2%") % aServer % aPort));
        LogManager::getInstance()->message(str(F_("Blocked a C-C connection to a hub ('%1%:%2%'; request from '%3%')") % aServer % aPort % aHubUrl));
        return true;
    }
    return false;
}

} // namespace dcpp
