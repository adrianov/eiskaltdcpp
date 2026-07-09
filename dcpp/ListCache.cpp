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
#include "Util.h"

namespace dcpp {

namespace {

const time_t DAY_SECS = 24 * 60 * 60;

string shareSizePath(const string& listBase) {
    return listBase + ".sharesize";
}

string fetchTimePath(const string& listBase) {
    return listBase + ".listfetch";
}

} // namespace

string ListCache::findListFile(const string& listBase) {
    if(File::getSize(listBase + ".xml.bz2") != -1)
        return listBase + ".xml.bz2";
    if(File::getSize(listBase + ".xml") != -1)
        return listBase + ".xml";
    return Util::emptyString;
}

int64_t ListCache::readShareSize(const string& listBase) {
    const string path = shareSizePath(listBase);
    if(File::getSize(path) == -1)
        return -1;
    try {
        return Util::toInt64(File(path, File::READ, File::OPEN).read());
    } catch(const Exception&) {
        return -1;
    }
}

void ListCache::saveShareSize(const string& listBase, int64_t shareSize) {
    try {
        File f(shareSizePath(listBase), File::WRITE, File::TRUNCATE | File::CREATE);
        f.write(Util::toString(shareSize));
    } catch(const FileException&) { }
}

bool ListCache::matchesUserShare(const HintedUser& user, const string& listBase) {
    if(!user.user->isOnline() || findListFile(listBase).empty())
        return false;

    const int64_t saved = readShareSize(listBase);
    if(saved < 0)
        return false;

    return ClientManager::getInstance()->getBytesShared(user.user) == saved;
}

time_t ListCache::readFetchTime(const string& listBase) {
    const string path = fetchTimePath(listBase);
    if(File::getSize(path) == -1)
        return -1;
    try {
        return static_cast<time_t>(Util::toInt64(File(path, File::READ, File::OPEN).read()));
    } catch(const Exception&) {
        return -1;
    }
}

void ListCache::saveFetchTime(const string& listBase, time_t when) {
    try {
        File f(fetchTimePath(listBase), File::WRITE, File::TRUNCATE | File::CREATE);
        f.write(Util::toString(static_cast<int64_t>(when)));
    } catch(const FileException&) { }
}

bool ListCache::fetchedWithinDay(const string& listBase) {
    const time_t fetched = readFetchTime(listBase);
    if(fetched < 0)
        return false;
    return (time(nullptr) - fetched) < DAY_SECS;
}

} // namespace dcpp
