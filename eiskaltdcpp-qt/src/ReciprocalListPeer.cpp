/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef USE_QT_SQLITE

#include "ReciprocalListPeer.h"

#include "dcpp/stdinc.h"
#include "dcpp/HintedUser.h"
#include "dcpp/ConnectionManagerPeerMatch.h"
#include "dcpp/ListCache.h"
#include "dcpp/QueueManager.h"

#include <QMutex>
#include <QMutexLocker>

#include <ctime>
#include <unordered_map>
#include <unordered_set>

using namespace dcpp;

namespace {

const time_t kPeerCooldownSecs = 10 * 60;

QMutex gPeerMutex;
std::unordered_map<string, time_t> gPeerChecked;

} // namespace

namespace ReciprocalListPeer {

bool cooldownActive(const HintedUser &peer)
{
    std::unordered_set<string> keys;
    ConnectionManagerPeerMatch::collectListPeerKeys(peer, keys);
    const time_t now = time(nullptr);
    QMutexLocker lock(&gPeerMutex);
    for (const auto &key : keys) {
        const auto it = gPeerChecked.find(key);
        if (it != gPeerChecked.end() && (now - it->second) < kPeerCooldownSecs)
            return true;
    }
    return false;
}

void markChecked(const HintedUser &peer)
{
    std::unordered_set<string> keys;
    ConnectionManagerPeerMatch::collectListPeerKeys(peer, keys);
    const time_t now = time(nullptr);
    QMutexLocker lock(&gPeerMutex);
    if (gPeerChecked.size() > 512) {
        for (auto it = gPeerChecked.begin(); it != gPeerChecked.end(); ) {
            if ((now - it->second) >= kPeerCooldownSecs)
                it = gPeerChecked.erase(it);
            else
                ++it;
        }
    }
    for (const auto &key : keys)
        gPeerChecked[key] = now;
}

bool findCachedList(const HintedUser &peer, string &listFile)
{
    const string listBase = QueueManager::getInstance()->getListPath(peer);
    listFile = ListCache::findListFile(listBase);
    if (ListCache::matchesUserShare(peer, listBase))
        return true;

    bool found = false;
    ConnectionManagerPeerMatch::forEachListPeer(peer, [&](const HintedUser &alias) {
        if (found || alias.user == peer.user)
            return;
        const string base = QueueManager::getInstance()->getListPath(alias);
        if (ListCache::matchesUserShare(alias, base))
            found = true;
    });
    return found;
}

} // namespace ReciprocalListPeer

#endif
