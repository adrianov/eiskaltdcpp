/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "ListCache.h"

#include "ClientManager.h"
#include "ConnectionManagerPeerMatch.h"
#include "DirectoryListing.h"
#include "File.h"
#include "ListCacheStore.h"
#include "Util.h"

namespace dcpp {

namespace {

const time_t DAY_SECS = 24 * 60 * 60;

} // namespace

void ListCache::load() {
    ListCacheStore::load();
}

string ListCache::findListFile(const string& listBase) {
    if(File::getSize(listBase + ".xml.bz2") != -1)
        return listBase + ".xml.bz2";
    if(File::getSize(listBase + ".xml") != -1)
        return listBase + ".xml";
    return Util::emptyString;
}

bool ListCache::matchesShare(const UserPtr& user) {
    if(!user || !user->isOnline())
        return false;

    const int64_t saved = ListCacheStore::shareSize(user->getCID());
    if(saved < 0)
        return false;

    return ClientManager::getInstance()->getBytesShared(user) == saved;
}

bool ListCache::isPlausibleList(int64_t listSize) {
    return listSize > 0;
}

bool ListCache::listHasEntries(const HintedUser& user, const string& listPath) {
    try {
        DirectoryListing dl(user);
        dl.loadFile(listPath);
        const DirectoryListing::Directory* root = dl.getRoot();
        return root && (!root->files.empty() || !root->directories.empty());
    } catch(const Exception&) {
        return false;
    }
}

bool ListCache::matchesUserShare(const HintedUser& user, const string& listBase) {
    if(!matchesShare(user.user))
        return false;
    const string path = findListFile(listBase);
    if(path.empty())
        return false;
    return isPlausibleList(File::getSize(path)) && listHasEntries(user, path);
}

int64_t ListCache::fileSize(const CID& cid) {
    return ListCacheStore::fileSize(cid);
}

void ListCache::saveListMeta(const CID& cid, int64_t shareSize, int64_t listSize, time_t when) {
    ListCacheStore::setMeta(cid, shareSize, listSize, when);
}

bool ListCache::fetchedWithinDay(const CID& cid) {
    const time_t fetched = ListCacheStore::fetchTime(cid);
    if(fetched < 0)
        return false;
    return (time(nullptr) - fetched) < DAY_SECS;
}

bool ListCache::fetchedWithinDay(const HintedUser& user) {
    if(!user.user)
        return false;
    if(fetchedWithinDay(user.user->getCID()))
        return true;
    bool within = false;
    ConnectionManagerPeerMatch::forEachListPeer(user, [&](const HintedUser& peer) {
        if(within || peer.user->getCID() == user.user->getCID())
            return;
        if(fetchedWithinDay(peer.user->getCID()))
            within = true;
    });
    return within;
}

} // namespace dcpp
