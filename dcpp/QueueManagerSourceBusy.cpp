/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "QueueManager.h"

#include "ClientManager.h"
#include "ConnectionManager.h"
#include "Download.h"
#include "UserConnection.h"

namespace dcpp {

namespace {

string peerIp(const HintedUser& hu, const QueueItem* qi) {
    for(auto& d: qi->getDownloads()) {
        if(d->getUser() == hu.user) {
            const string ip = Util::normalizeIpv4(d->getUserConnection().getRemoteIp());
            if(!ip.empty())
                return ip;
        }
    }
    auto* cm = ClientManager::getInstance();
    string ip = Util::normalizeIpv4(cm->getField(hu.user->getCID(), hu.hint, "I4"));
    if(ip.empty())
        ip = Util::normalizeIpv4(cm->getField(hu.user->getCID(), Util::emptyString, "I4"));
    return ip;
}

bool peersSameNick(const HintedUser& a, const HintedUser& b) {
    const StringList na = ClientManager::getInstance()->getNicks(a);
    const StringList nb = ClientManager::getInstance()->getNicks(b);
    return !na.empty() && !nb.empty() && Util::stricmp(na[0].c_str(), nb[0].c_str()) == 0;
}

} // namespace

bool QueueManager::isBusyOnFile(const QueueItem* qi, const HintedUser& src,
        const QueuedDownloadUsers& queued) {
    if(!qi || !src.user)
        return false;
    if(userQueue.getRunning(src.user) == qi)
        return true;
    for(auto& d: qi->getDownloads()) {
        if(d->getUser() == src.user)
            return true;
    }
    if(!qi->isSource(src.user))
        return false;
    if(queued.count(src.user->getCID()) &&
            userQueue.getNext(src.user, QueueItem::LOWEST) == qi)
        return true;
    return false;
}

bool QueueManager::hasBusyAlias(const QueueItem* qi, const HintedUser& candidate,
        const QueuedDownloadUsers& queued) {
    if(!qi || !candidate.user)
        return false;
    const string candIp = peerIp(candidate, qi);
    for(auto& s: qi->getSources()) {
        const HintedUser& other = s.getUser();
        if(other.user == candidate.user || !isBusyOnFile(qi, other, queued))
            continue;
        if(peersSameNick(other, candidate))
            return true;
        const string otherIp = peerIp(other, qi);
        if(!candIp.empty() && candIp == otherIp)
            return true;
    }
    return false;
}

bool QueueManager::shouldConnectSource(const QueueItem* qi, const HintedUser& aUser,
        const QueuedDownloadUsers& queued) noexcept {
    Lock l(cs);
    if(!qi || qi->isFinished() || qi->getPriority() == QueueItem::PAUSED)
        return false;
    return !hasBusyAlias(qi, aUser, queued);
}

bool QueueManager::allowDownloadConnect(const HintedUser& aUser) noexcept {
    const auto queued = ConnectionManager::getInstance()->queuedDownloadUsers();
    Lock l(cs);
    if(!aUser.user)
        return false;
    if(userQueue.getRunning(aUser))
        return true;
    QueueItem* qi = userQueue.getNext(aUser.user, QueueItem::LOWEST);
    return qi && !hasBusyAlias(qi, aUser, queued);
}

} // namespace dcpp
