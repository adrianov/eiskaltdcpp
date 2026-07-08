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
#include "FavoriteManager.h"
#include "MappingManager.h"
#include "PeerConnectFilter.h"
#include "PeerConnectLog.h"
#include "QueueManager.h"

#include <unordered_set>

namespace dcpp {

void ConnectionManager::getDownloadConnection(const HintedUser& aUser) {
    dcassert((bool)aUser.user);
    {
        Lock l(cs);

        StringList nicks = ClientManager::getInstance()->getNicks(aUser);
        if(!nicks.empty()) {
            std::unordered_set<CID> sameNick;
            ClientManager::getInstance()->cidsForNick(nicks[0], sameNick);
            ConnectionQueueItem::List stale;
            for(auto& cqi : downloads) {
                if(cqi->getUser().user == aUser.user)
                    continue;
                if(sameNick.count(cqi->getUser().user->getCID()))
                    stale.push_back(cqi);
            }
            for(auto& cqi : stale)
                putCQI(cqi);
        }

        auto i = find(downloads.begin(), downloads.end(), aUser.user);
        if(i == downloads.end()) {
            getCQI(aUser, true);
        } else {
            DownloadManager::getInstance()->checkIdle(aUser.user);
        }
    }
}

ConnectionQueueItem* ConnectionManager::getCQI(const HintedUser& user, bool download) {
    ConnectionQueueItem* cqi = new ConnectionQueueItem(user, download);
    if(download) {
        dcassert(find(downloads.begin(), downloads.end(), user.user) == downloads.end());
        downloads.push_back(cqi);
    } else {
        dcassert(find(uploads.begin(), uploads.end(), user.user) == uploads.end());
        uploads.push_back(cqi);
    }

    fire(ConnectionManagerListener::Added(), cqi);
    if(download)
        PeerConnectLog::queueAdded(user);
    return cqi;
}

void ConnectionManager::putCQI(ConnectionQueueItem* cqi) {
    fire(ConnectionManagerListener::Removed(), cqi);
    if(cqi->getDownload()) {
        dcassert(find(downloads.begin(), downloads.end(), cqi) != downloads.end());
        downloads.erase(remove(downloads.begin(), downloads.end(), cqi), downloads.end());
    } else {
        dcassert(find(uploads.begin(), uploads.end(), cqi) != uploads.end());
        uploads.erase(remove(uploads.begin(), uploads.end(), cqi), uploads.end());
    }
    delete cqi;
}

void ConnectionManager::onUpnpReady() {
    Lock l(cs);
    for(auto& cqi : downloads) {
        if(cqi->getState() != ConnectionQueueItem::ACTIVE)
            cqi->setLastAttempt(0);
    }
}

void ConnectionManager::on(TimerManagerListener::Second, uint64_t aTick) noexcept {
    UserList passiveUsers;
    ConnectionQueueItem::List removed;

    {
        Lock l(cs);

        bool attemptDone = false;
        const bool upnpReady = MappingManager::getInstance()->readyForPeerConnect();

        for(auto& cqi: downloads) {

            if(cqi->getState() != ConnectionQueueItem::ACTIVE) {
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

                if(cqi->getErrors() == -1 && cqi->getLastAttempt() != 0) {
                    continue;
                }

                if(PeerConnectFilter::shouldGiveUp(cqi->getErrors())) {
                    markQueueGiveUp(cqi, cqi->getErrors());
                    continue;
                }

                if(cqi->getLastAttempt() == 0 || (!attemptDone &&
                                                  cqi->getLastAttempt() + PeerConnectFilter::queueBackoffMs(cqi->getErrors(), cqi->getSlotWaits()) < aTick))
                {
                    if(!upnpReady)
                        continue;

                    if(DownloadManager::getInstance()->isWaitingUploadSlot(cqi->getUser().user))
                        continue;

                    cqi->setLastAttempt(aTick);

                    QueueItem::Priority prio = QueueManager::getInstance()->hasDownload(cqi->getUser());

                    if(prio == QueueItem::PAUSED) {
                        removed.push_back(cqi);
                        continue;
                    }

                    bool startDown = DownloadManager::getInstance()->startDownload(prio);

                    if(cqi->getState() == ConnectionQueueItem::WAITING) {
                        if(startDown) {
                            cqi->setState(ConnectionQueueItem::CONNECTING);
                            cqi->setConnectAttempts(cqi->getConnectAttempts() + 1);

                            if(cqi->getConnectAttempts() >= 3) {
                                OnlineUser* ou = ClientManager::getInstance()->findBestOnlineUser(
                                        cqi->getUser().user->getCID(), cqi->getUser().hint,
                                        FavoriteManager::getInstance()->isPrivate(cqi->getUser().hint));
                                if(ou && ou->getClient().getHubUrl() != cqi->getUser().hint)
                                    cqi->setHubHint(ou->getClient().getHubUrl());
                            }

                            const bool reverseConnect = ClientManager::getInstance()->wantRevConnect(cqi->getUser(), cqi->getConnectAttempts());
                            PeerConnectLog::queueStart(cqi->getUser());
                            ClientManager::getInstance()->connect(cqi->getUser(), cqi->getToken(), reverseConnect, cqi->getSecureMode());
                            fire(ConnectionManagerListener::StatusChanged(), cqi);
                            attemptDone = true;
                        } else {
                            cqi->setState(ConnectionQueueItem::NO_DOWNLOAD_SLOTS);
                            fire(ConnectionManagerListener::Failed(), cqi, _("All download slots taken"));
                        }
                    } else if(cqi->getState() == ConnectionQueueItem::NO_DOWNLOAD_SLOTS && startDown) {
                        cqi->setState(ConnectionQueueItem::WAITING);
                    }
                } else if(cqi->getState() == ConnectionQueueItem::CONNECTING && cqi->getLastAttempt() + 50 * 1000 < aTick) {
                    cqi->setErrors(cqi->getErrors() + 1);
                    if(PeerConnectFilter::shouldGiveUp(cqi->getErrors())) {
                        markQueueGiveUp(cqi, cqi->getErrors());
                        cqi->setState(ConnectionQueueItem::WAITING);
                    } else {
                        PeerConnectLog::queueTimeout(cqi->getUser(), cqi->getErrors());
                        fire(ConnectionManagerListener::Failed(), cqi, _("Connection timeout"));
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
