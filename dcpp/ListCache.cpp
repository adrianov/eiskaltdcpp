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

bool ListCache::matchesUserShare(const HintedUser& user, const string& listBase) {
    if(!user.user->isOnline() || findListFile(listBase).empty())
        return false;

    const int64_t saved = ListCacheStore::shareSize(user.user->getCID());
    if(saved < 0)
        return false;

    return ClientManager::getInstance()->getBytesShared(user.user) == saved;
}

void ListCache::saveListMeta(const CID& cid, int64_t shareSize, time_t when) {
    ListCacheStore::setMeta(cid, shareSize, when);
}

bool ListCache::fetchedWithinDay(const CID& cid) {
    const time_t fetched = ListCacheStore::fetchTime(cid);
    if(fetched < 0)
        return false;
    return (time(nullptr) - fetched) < DAY_SECS;
}

} // namespace dcpp
