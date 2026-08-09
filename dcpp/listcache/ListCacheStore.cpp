/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "listcache/ListCacheStore.h"

#include "Encoder.h"
#include "File.h"
#include "SimpleXML.h"
#include "Util.h"

#include <mutex>
#include <unordered_map>

namespace dcpp {
namespace ListCacheStore {
namespace {

struct Entry {
    int64_t shareSize = -1;
    int64_t fileSize = -1;
    time_t fetchTime = -1;
};

std::unordered_map<string, Entry> entries;
CriticalSection dataCs;
CriticalSection fileCs;
std::once_flag loadFlag;

string cacheFile() {
    return Util::getPath(Util::PATH_USER_LOCAL) + "ListCache.xml";
}

void writeXml() {
    Lock fileLock(fileCs);
    SimpleXML xml;
    xml.addTag("ListCache");
    xml.stepIn();
    {
        Lock l(dataCs);
        for(const auto& i: entries) {
            if(i.second.shareSize < 0 && i.second.fileSize < 0 && i.second.fetchTime < 0)
                continue;
            xml.addTag("User");
            xml.addChildAttrib("CID", i.first);
            if(i.second.shareSize >= 0)
                xml.addChildAttrib("ShareSize", Util::toString(i.second.shareSize));
            if(i.second.fileSize >= 0)
                xml.addChildAttrib("FileSize", Util::toString(i.second.fileSize));
            if(i.second.fetchTime >= 0)
                xml.addChildAttrib("FetchTime", Util::toString(static_cast<int64_t>(i.second.fetchTime)));
        }
    }
    xml.stepOut();

    const string fName = cacheFile();
    File out(fName + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
    BufferedOutputStream<false> f(&out);
    f.write(SimpleXML::utf8Header);
    xml.toXML(&f);
    f.flush();
    out.close();
    File::renameFile(fName + ".tmp", fName);
}

void readXml() {
    if(File::getSize(cacheFile()) == -1)
        return;

    SimpleXML xml;
    xml.fromXML(File(cacheFile(), File::READ, File::OPEN).read());
    if(!xml.findChild("ListCache"))
        return;

    xml.stepIn();
    Lock l(dataCs);
    while(xml.findChild("User")) {
        const string cid = xml.getChildAttrib("CID");
        if(!isCid(cid))
            continue;

        Entry entry;
        const string size = xml.getChildAttrib("ShareSize");
        const string fileSize = xml.getChildAttrib("FileSize");
        const string time = xml.getChildAttrib("FetchTime");
        if(!size.empty())
            entry.shareSize = Util::toInt64(size);
        if(!fileSize.empty())
            entry.fileSize = Util::toInt64(fileSize);
        if(!time.empty())
            entry.fetchTime = static_cast<time_t>(Util::toInt64(time));
        entries[cid] = entry;
    }
}

void loadStore() {
    try {
        readXml();
    } catch(const Exception&) { }
}

} // namespace

void load() {
    std::call_once(loadFlag, loadStore);
}

bool isCid(const string& cid) {
    return cid.size() == 39 && Encoder::isBase32(cid);
}

time_t cachedFetch(const string& cid) {
    Lock l(dataCs);
    const auto it = entries.find(cid);
    return it == entries.end() ? -1 : it->second.fetchTime;
}

bool eraseCid(const string& cid) {
    Lock l(dataCs);
    return entries.erase(cid) > 0;
}

void mergeMigrated(const string& cid, int64_t shareSize, time_t fetchTime) {
    Lock l(dataCs);
    const auto old = entries.find(cid);
    if(old != entries.end() && fetchTime <= old->second.fetchTime)
        return;
    Entry& entry = entries[cid];
    entry.shareSize = shareSize;
    if(fetchTime >= 0)
        entry.fetchTime = fetchTime;
}

bool persist() {
    try {
        writeXml();
        return true;
    } catch(const Exception&) {
        return false;
    }
}

bool eraseFetchedBefore(time_t cutoff) {
    Lock l(dataCs);
    bool dirty = false;
    for(auto it = entries.begin(); it != entries.end(); ) {
        if(it->second.fetchTime >= 0 && it->second.fetchTime < cutoff) {
            it = entries.erase(it);
            dirty = true;
        } else {
            ++it;
        }
    }
    return dirty;
}

int64_t shareSize(const CID& cid) {
    load();
    Lock l(dataCs);
    const auto it = entries.find(cid.toBase32());
    return it == entries.end() ? -1 : it->second.shareSize;
}

int64_t fileSize(const CID& cid) {
    load();
    Lock l(dataCs);
    const auto it = entries.find(cid.toBase32());
    return it == entries.end() ? -1 : it->second.fileSize;
}

time_t fetchTime(const CID& cid) {
    load();
    Lock l(dataCs);
    const auto it = entries.find(cid.toBase32());
    return it == entries.end() ? -1 : it->second.fetchTime;
}

void setMeta(const CID& cid, int64_t shareSize, int64_t fileSize, time_t when) {
    load();
    {
        Lock l(dataCs);
        Entry& entry = entries[cid.toBase32()];
        entry.shareSize = shareSize;
        entry.fileSize = fileSize;
        entry.fetchTime = when;
    }
    persist();
}

} // namespace ListCacheStore

} // namespace dcpp
