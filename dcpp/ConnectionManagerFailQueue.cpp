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
#include "ConnectionManager.h"

#include "DownloadRetryPolicy.h"
#include "PeerConnectFilter.h"
#include "PeerConnectHub.h"
#include "PeerConnectLog.h"
#include "QueueManager.h"
#include "UserConnection.h"

namespace dcpp {

namespace {

/** Prefer hub-matched upload CQI so sim-upload disconnects drop the right Transfers row. */
ConnectionQueueItem* findUploadCqi(ConnectionQueueItem::List& uploads, UserConnection* uc) {
    if(!uc || !uc->getUser())
        return nullptr;
    ConnectionQueueItem* fallback = nullptr;
    const string& hub = uc->getHubUrl();
    for(auto* cqi : uploads) {
        if(cqi->getUser().user != uc->getUser())
            continue;
        if(!hub.empty() && cqi->getUser().hint == hub)
            return cqi;
        if(!fallback)
            fallback = cqi;
    }
    return fallback;
}

} // namespace

bool ConnectionManager::onDownloadConnectTimeout(ConnectionQueueItem* cqi) {
    cqi->setErrors(cqi->getErrors() + 1);
    // Unreachable now, whatever the peer answered last time.
    cqi->setQueuePos(-1);
    // Zero lastAttempt: full CONNECTING wait already elapsed — hub-rotate now.
    cqi->setLastAttempt(0);
    const string timedOutHub = cqi->getUser().hint;
    PeerConnectHub::noteConnectTimeout(cqi->getUser().user, timedOutHub);
    PeerConnectHub::rememberFailure(cqi->getUser().user, timedOutHub);
    clearOutgoingConnect(cqi->getUser().user);

    // Prefer another online identity of the same peer over giving up: NMDC has one
    // CID per hub, so a single miss already reports "all hubs timed out" for this
    // CID. The error count still limits how many attempts the peer gets in total.
    if(!switchDownloadIdentity(cqi)) {
        if(DownloadRetryPolicy::dropUnreachable(cqi)) {
            // Keep hub hint for removePeerSources(samePeer); clear Transfers via Failed.
            fire(ConnectionManagerListener::Failed(), cqi, _("Connection timeout"));
            return true;
        }
        // Drop hint so the next resolve does not soft-boost the hub that timed out.
        cqi->setHubHint(Util::emptyString);
    }

    if(PeerConnectFilter::shouldGiveUp(cqi->getErrors()))
        DownloadRetryPolicy::markGiveUp(cqi, cqi->getErrors(), false);
    else
        PeerConnectLog::queueTimeout(cqi->getUser(), cqi->getErrors());

    // Always Failed so the Transfers row is cleared (give-up used to leave "Connecting").
    fire(ConnectionManagerListener::Failed(), cqi, _("Connection timeout"));
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
    cqi->setQueuePos(-1);
    cqi->setLastAttempt(0);
    cqi->setConnectAttempts(0);
    clearOutgoingConnect(cqi->getUser().user);
}

void ConnectionManager::failDownloadQueue(ConnectionQueueItem* dlCqi, const DownloadRetryPolicy& policy,
        const string& aError) {
    // Nothing left to fetch (e.g. file list just finished): drop instead of
    // slot-wait / Failed UI that would stick as "Connection closed".
    if(QueueManager::getInstance()->hasDownload(dlCqi->getUser()) == QueueItem::PAUSED) {
        putCQI(dlCqi);
        return;
    }

    policy.apply(dlCqi);
    fire(ConnectionManagerListener::Failed(), dlCqi, aError);
}

void ConnectionManager::failed(UserConnection* aSource, const string& aError, bool protocolError) {
    Lock l(cs);

    ConnectionQueueItem::Ptr dlCqi = nullptr;
    if(aSource->getUser()) {
        auto i = find(downloads.begin(), downloads.end(), aSource->getUser());
        dlCqi = i != downloads.end() ? *i : findDownloadCqi(aSource->getHintedUser());
    }

    const DownloadRetryPolicy policy(aSource, aError, protocolError);
    if(dlCqi && aSource->getUser())
        policy.logFail(dlCqi->getErrors());

    if(dlCqi && aSource->isSet(UserConnection::FLAG_DOWNLOAD)) {
        failDownloadQueue(dlCqi, policy, aError);
    } else if(aSource->isSet(UserConnection::FLAG_ASSOCIATED) &&
            aSource->isSet(UserConnection::FLAG_UPLOAD)) {
        if(auto* ulCqi = findUploadCqi(uploads, aSource))
            putCQI(ulCqi);
    }
    putConnection(aSource);
}

} // namespace dcpp
