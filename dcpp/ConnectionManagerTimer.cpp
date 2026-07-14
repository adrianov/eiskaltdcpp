/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
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
#include "MappingManager.h"
#include "PeerConnectFilter.h"
#include "PeerConnectLog.h"
#include "QueueManager.h"

namespace dcpp {

void ConnectionManager::on(TimerManagerListener::Second, uint64_t aTick) noexcept {
    UserList passiveUsers;
    ConnectionQueueItem::List removed;

    {
        Lock l(cs);

        bool attemptDone = false;
        const bool upnpReady = MappingManager::getInstance()->readyForPeerConnect();

        for(auto& cqi: downloads) {

            if(cqi->getState() != ConnectionQueueItem::ACTIVE) {
                if(cqi->getUser().user->isSet(User::NMDC) && Util::toInt64(
                        ClientManager::getInstance()->getField(cqi->getUser().user->getCID(),
                        cqi->getUser().hint, "SS")) <= 0) {
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
                if(QueueManager::getInstance()->hasDownload(cqi->getUser()) == QueueItem::PAUSED) {
                    removed.push_back(cqi);
                    continue;
                }

                if(cqi->getErrors() == -1) {
                    continue;
                }

                if(queueBackoffActive(cqi)) {
                    continue;
                }

                if(PeerConnectFilter::shouldGiveUp(cqi->getErrors())) {
                    markQueueGiveUp(cqi, cqi->getErrors(), false);
                    continue;
                }

                if(cqi->getState() == ConnectionQueueItem::CONNECTING) {
                    if(cqi->getLastAttempt() + 50 * 1000 < aTick) {
                        cqi->setErrors(cqi->getErrors() + 1);
                        cqi->setLastAttempt(aTick);
                        noteConnectCooldown(cqi->getUser().user,
                                PeerConnectFilter::connectBackoffMs(cqi->getErrors()));
                        if(PeerConnectFilter::shouldGiveUp(cqi->getErrors())) {
                            markQueueGiveUp(cqi, cqi->getErrors(), false);
                        } else {
                            PeerConnectLog::queueTimeout(cqi->getUser(), cqi->getErrors());
                            fire(ConnectionManagerListener::Failed(), cqi, _("Connection timeout"));
                        }
                        cqi->setState(ConnectionQueueItem::WAITING);
                    }
                    continue;
                }

                if(cqi->getLastAttempt() == 0 || !attemptDone)
                {
                    if(!upnpReady)
                        continue;

                    if(DownloadManager::getInstance()->isWaitingUploadSlot(cqi->getUser().user))
                        continue;

                    if(!QueueManager::getInstance()->allowDownloadConnect(cqi->getUser()))
                        continue;

                    QueueItem::Priority prio = QueueManager::getInstance()->hasDownload(cqi->getUser());
                    bool startDown = DownloadManager::getInstance()->startDownload(prio);

                    if(cqi->getState() == ConnectionQueueItem::WAITING) {
                        if(startDown) {
                            cqi->setLastAttempt(aTick);
                            cqi->setState(ConnectionQueueItem::CONNECTING);
                            cqi->setConnectAttempts(cqi->getConnectAttempts() + 1);

                            if(cqi->getUser().hint.empty()) {
                                const string hub = ClientManager::getInstance()->resolveHubHint(
                                        cqi->getUser().user);
                                if(!hub.empty())
                                    cqi->setHubHint(hub);
                            }

                            const bool reverseConnect = ClientManager::getInstance()->wantRevConnect(
                                    cqi->getUser(), cqi->getConnectAttempts());
                            PeerConnectLog::queueStart(cqi->getUser());
                            if(ClientManager::getInstance()->connect(cqi->getUser(), cqi->getToken(),
                                    reverseConnect, cqi->getSecureMode())) {
                                fire(ConnectionManagerListener::StatusChanged(), cqi);
                                attemptDone = true;
                            } else {
                                cqi->setState(ConnectionQueueItem::WAITING);
                            }
                        } else {
                            cqi->setState(ConnectionQueueItem::NO_DOWNLOAD_SLOTS);
                            fire(ConnectionManagerListener::Failed(), cqi, _("All download slots taken"));
                        }
                    } else if(cqi->getState() == ConnectionQueueItem::NO_DOWNLOAD_SLOTS && startDown) {
                        cqi->setState(ConnectionQueueItem::WAITING);
                    }
                }
            }
        }

        for(auto& m : removed) {
            putCQI(m);
        }

    }

    for(auto& ui : passiveUsers) {
        QueueManager::getInstance()->removeSource(ui, QueueItem::Source::FLAG_PASSIVE);
    }
}

} // namespace dcpp
