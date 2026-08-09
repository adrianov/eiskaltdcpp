/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "PeerConnectAttempt.h"

#include "ClientManager.h"
#include "DownloadManager.h"
#include "FavoriteManager.h"
#include "PeerConnectLog.h"
#include "QueueManager.h"

namespace dcpp {

namespace {

/** Peers worth a download slot right now: not already queued by them, and not
 *  advertising zero free upload slots. NMDC has no free slot field (the tag "S:"
 *  is the total), so an unknown count stays in the preferred group. */
bool mayGrantSlot(const ConnectionQueueItem* cqi) {
    if(cqi->getSlotWaits() > 0)
        return false;
    const string freeSlots = ClientManager::getInstance()->getField(
            cqi->getUser().user->getCID(), cqi->getUser().hint, "FS");
    return freeSlots.empty() || Util::toInt(freeSlots) > 0;
}

} // namespace

PeerConnectAttempt::PeerConnectAttempt(ConnectionQueueItem* aCqi, uint64_t aTick) noexcept :
    cqi(aCqi),
    tick(aTick)
{
}

void PeerConnectAttempt::preferFreeSlots(ConnectionQueueItem::List& order) {
    std::stable_partition(order.begin(), order.end(), mayGrantSlot);
}

bool PeerConnectAttempt::ready(const HintedUser& user) {
    return !DownloadManager::getInstance()->isWaitingUploadSlot(user.user) &&
            QueueManager::getInstance()->allowDownloadConnect(user);
}

bool PeerConnectAttempt::start() const {
    auto* cm = ClientManager::getInstance();
    // Best hub already prefers non-waiting hubs; if it is still waiting, skip
    // without lastAttempt so the timer retries as soon as the pause ends.
    const bool priv = FavoriteManager::getInstance()->isPrivate(cqi->getUser().hint);
    if(OnlineUser* ou = cm->findBestOnlineUser(cqi->getUser().user->getCID(),
            cqi->getUser().hint, priv)) {
        if(!ou->getClient().allowHubConnect()) {
            PeerConnectLog::skip(ou->getIdentity().getNick(), ou->getClient().getHubUrl(),
                    _("hub peer-connect wait"));
            return false;
        }
    }

    cqi->setLastAttempt(tick);
    cqi->setConnectAttempts(cqi->getConnectAttempts() + 1);

    const string hub = cm->resolveHubHint(cqi->getUser().user, cqi->getUser().hint);
    if(!hub.empty())
        cqi->setHubHint(hub);

    const bool reverseConnect = cm->wantRevConnect(cqi->getUser(), cqi->getConnectAttempts());
    string usedHub;
    if(!cm->connect(cqi->getUser(), cqi->getToken(), reverseConnect, cqi->getSecureMode(),
            &usedHub)) {
        // Stay WAITING; keep lastAttempt for queueBackoffMs pacing.
        if(cqi->getConnectAttempts() > 0)
            cqi->setConnectAttempts(cqi->getConnectAttempts() - 1);
        return false;
    }

    // CONNECTING only after connect() accepts — otherwise allowOutgoingConnect
    // sees our own item as "still in flight".
    cqi->setState(ConnectionQueueItem::CONNECTING);
    if(!usedHub.empty())
        cqi->setHubHint(usedHub);
    PeerConnectLog::queueStart(cqi->getUser());
    return true;
}

} // namespace dcpp
