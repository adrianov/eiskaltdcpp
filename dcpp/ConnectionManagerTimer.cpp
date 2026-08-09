/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "ConnectionManager.h"

#include "ClientManager.h"
#include "DownloadManager.h"
#include "DownloadRetryPolicy.h"
#include "MappingManager.h"
#include "PeerConnectAttempt.h"
#include "PeerConnectFilter.h"
#include "PeerConnectLog.h"
#include "QueueManager.h"

namespace dcpp {

void ConnectionManager::on(TimerManagerListener::Second, uint64_t aTick) noexcept {
    UserList passiveUsers;
    HintedUserList unreachableUsers;
    ConnectionQueueItem::List removed;

    {
        Lock l(cs);

        bool attemptDone = false;
        const bool upnpReady = MappingManager::getInstance()->readyForPeerConnect();

        // Spend the connect attempt on a peer that can start now, not on one that
        // will answer MaxedOut. Removals stay deferred, so a copy is safe to walk.
        ConnectionQueueItem::List order = downloads;
        PeerConnectAttempt::preferFreeSlots(order);

        for(auto& cqi: order) {
            if(cqi->getState() == ConnectionQueueItem::ACTIVE)
                continue;

            if(cqi->getUser().user->isSet(User::NMDC) && Util::toInt64(
                    ClientManager::getInstance()->getField(cqi->getUser().user->getCID(),
                    cqi->getUser().hint, "SS")) <= 0) {
                waitPeerInfo(cqi->getUser().user);
                removed.push_back(cqi);
                continue;
            }
            if(cqi->getState() == ConnectionQueueItem::WAITING) {
                auto* alias = findDownloadCqi(cqi->getUser());
                if(alias && alias != cqi && (alias->getState() == ConnectionQueueItem::ACTIVE ||
                        alias->getState() == ConnectionQueueItem::CONNECTING)) {
                    mergeQueueState(alias, cqi);
                    removed.push_back(cqi);
                    continue;
                }
                // Only skipped hubs still online (others left) — drop as unreachable.
                // Hub identity rotation already happened on connect timeout.
                if(DownloadRetryPolicy::dropUnreachable(cqi)) {
                    unreachableUsers.push_back(cqi->getUser());
                    removed.push_back(cqi);
                    continue;
                }
            }
            if(!cqi->getUser().user->isOnline()) {
                removed.push_back(cqi);
                continue;
            }

            if(cqi->getUser().user->isSet(User::PASSIVE) && !ClientManager::getInstance()->isActive()) {
                PeerConnectLog::passiveWait(cqi->getUser());
                passiveUsers.push_back(cqi->getUser());
                removed.push_back(cqi);
                continue;
            }

            // Drop idle CQIs before give-up/backoff skips (finished file list, etc.).
            const QueueItem::Priority prio = QueueManager::getInstance()->hasDownload(cqi->getUser());
            if(prio == QueueItem::PAUSED) {
                removed.push_back(cqi);
                continue;
            }

            if(cqi->getErrors() == -1) {
                reviveDownloadQueue(cqi);
                if(cqi->getErrors() == -1)
                    continue;
            }

            // CONNECTING timeout before WAITING backoff so hub rotate is not delayed.
            if(cqi->getState() == ConnectionQueueItem::CONNECTING) {
                // lastAttempt==0 is invalid while CONNECTING; recover instead of instant timeout.
                if(cqi->getLastAttempt() == 0) {
                    cqi->setLastAttempt(aTick);
                    continue;
                }
                if(cqi->getLastAttempt() + PeerConnectFilter::CONNECT_TIMEOUT_MS < aTick) {
                    if(onDownloadConnectTimeout(cqi)) {
                        unreachableUsers.push_back(cqi->getUser());
                        removed.push_back(cqi);
                    }
                }
                continue;
            }

            const bool startDown = DownloadManager::getInstance()->startDownload(prio);
            if(cqi->getState() == ConnectionQueueItem::NO_DOWNLOAD_SLOTS) {
                if(!startDown)
                    continue;
                cqi->setState(ConnectionQueueItem::WAITING);
            }

            if(queueBackoffActive(cqi))
                continue;

            if(PeerConnectFilter::shouldGiveUp(cqi->getErrors())) {
                if(DownloadRetryPolicy::dropUnreachable(cqi)) {
                    unreachableUsers.push_back(cqi->getUser());
                    removed.push_back(cqi);
                    continue;
                }
                DownloadRetryPolicy::markGiveUp(cqi, cqi->getErrors(), false);
                continue;
            }

            if(cqi->getLastAttempt() != 0 && attemptDone)
                continue;

            if(!upnpReady || !PeerConnectAttempt::ready(cqi->getUser()) ||
                    peerConnectInFlight(cqi->getUser()))
                continue;

            if(!startDown) {
                cqi->setState(ConnectionQueueItem::NO_DOWNLOAD_SLOTS);
                cqi->setLastAttempt(0);
                // Our limit, not the peer's: drop any remembered slot wait.
                cqi->setQueuePos(-1);
                fire(ConnectionManagerListener::Failed(), cqi, _("All download slots taken"));
                continue;
            }

            // Rotate NMDC hub identities / ADC hub hints before CTM.
            switchDownloadIdentity(cqi);
            if(PeerConnectAttempt(cqi, aTick).start()) {
                fire(ConnectionManagerListener::StatusChanged(), cqi);
                attemptDone = true;
            }
        }

        for(auto& m : removed) {
            putCQI(m);
        }

    }

    for(auto& ui : passiveUsers) {
        QueueManager::getInstance()->removeSource(ui, QueueItem::Source::FLAG_PASSIVE);
    }
    for(auto& ui : unreachableUsers) {
        QueueManager::getInstance()->removePeerSources(ui, QueueItem::Source::FLAG_UNREACHABLE);
    }
}

} // namespace dcpp
