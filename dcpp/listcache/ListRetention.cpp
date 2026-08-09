/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "listcache/ListRetention.h"

#include "File.h"
#include "Util.h"
#include "listcache/ListCacheStore.h"

namespace dcpp {

namespace {

const string SIZE_EXT = ".sharesize";
const string FETCH_EXT = ".listfetch";
const size_t CID_LEN = 39;

void stripListExt(string& name) {
    if(name.size() > 4 && Util::stricmp(name.c_str() + name.size() - 4, ".bz2") == 0)
        name.erase(name.size() - 4);
    if(name.size() > 4 && Util::stricmp(name.c_str() + name.size() - 4, ".xml") == 0)
        name.erase(name.size() - 4);
    else if(name.size() > 6 && Util::stricmp(Util::getFileExt(name).c_str(), ".DcLst") == 0)
        name.erase(name.size() - Util::getFileExt(name).size());
}

} // namespace

void ListRetention::onStartup() {
    ListCacheStore::load();
    const StringList migrated = migrateSidecars();
    // Keep sidecars on disk if the centralized XML write fails.
    if(!migrated.empty() && ListCacheStore::persist()) {
        for(const auto& path: migrated)
            File::deleteFile(path);
    }
    expire();
}

void ListRetention::expire() {
    ListCacheStore::load();
    cutoff = time(nullptr) - maxAge;
    bool dirty = false;
    const string dir = Util::getListPath();
    for(const auto& path: File::findFiles(dir, "*.xml*"))
        dirty = expirePath(path) || dirty;
    for(const auto& path: File::findFiles(dir, "*.DcLst"))
        dirty = expirePath(path) || dirty;
    dirty = pruneMeta() || dirty;
    if(dirty)
        ListCacheStore::persist();
}

StringList ListRetention::migrateSidecars() {
    StringList migrated;
    for(const auto& sizePath: File::findFiles(Util::getListPath(), "*" + SIZE_EXT)) {
        const string name = Util::getFileName(sizePath);
        if(name.size() < CID_LEN + SIZE_EXT.size())
            continue;
        const string cid = name.substr(name.size() - CID_LEN - SIZE_EXT.size(), CID_LEN);
        if(!ListCacheStore::isCid(cid))
            continue;

        int64_t shareSize = -1;
        try {
            shareSize = Util::toInt64(File(sizePath, File::READ, File::OPEN).read());
        } catch(const Exception&) {
            continue;
        }

        time_t fetchTime = -1;
        const string fetchPath = sizePath.substr(0, sizePath.size() - SIZE_EXT.size()) + FETCH_EXT;
        try {
            if(File::getSize(fetchPath) != -1)
                fetchTime = static_cast<time_t>(Util::toInt64(File(fetchPath, File::READ, File::OPEN).read()));
        } catch(const Exception&) { }

        ListCacheStore::mergeMigrated(cid, shareSize, fetchTime);
        migrated.push_back(sizePath);
        migrated.push_back(fetchPath);
    }
    return migrated;
}

bool ListRetention::expirePath(const string& path) {
    const string cid = cidFromPath(path);
    if(ageOf(path, cid) >= cutoff)
        return false;
    File::deleteFile(path);
    return !cid.empty() && ListCacheStore::eraseCid(cid);
}

bool ListRetention::pruneMeta() {
    return ListCacheStore::eraseFetchedBefore(cutoff);
}

string ListRetention::cidFromPath(const string& path) {
    string name = Util::getFileName(path);
    stripListExt(name);
    const auto dot = name.rfind('.');
    if(dot == string::npos)
        return Util::emptyString;
    const string cid = name.substr(dot + 1);
    return ListCacheStore::isCid(cid) ? cid : Util::emptyString;
}

time_t ListRetention::fileMtime(const string& path) {
    try {
        return File(path, File::READ, File::OPEN).getLastModified();
    } catch(const Exception&) {
        return 0;
    }
}

time_t ListRetention::ageOf(const string& path, const string& cid) const {
    if(!cid.empty()) {
        const time_t fetched = ListCacheStore::cachedFetch(cid);
        if(fetched >= 0)
            return fetched;
    }
    return fileMtime(path);
}

} // namespace dcpp
