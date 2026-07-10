/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "ListCacheStore.h"

#include "Encoder.h"
#include "File.h"
#include "SimpleXML.h"
#include "Util.h"

#include <mutex>
#include <unordered_map>
namespace dcpp {
namespace ListCacheStore {
namespace {
const size_t CID_LEN = 39;
const string SIZE_EXT = ".sharesize";
const string FETCH_EXT = ".listfetch";
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

bool validCid(const string& cid) {
    return cid.size() == CID_LEN && Encoder::isBase32(cid);
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
        if(!validCid(cid))
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
StringList migrateSidecars() {
    StringList migrated;
    for(const auto& sizePath: File::findFiles(Util::getListPath(), "*" + SIZE_EXT)) {
        const string name = Util::getFileName(sizePath);
        if(name.size() < CID_LEN + SIZE_EXT.size())
            continue;
        const string cid = name.substr(name.size() - CID_LEN - SIZE_EXT.size(), CID_LEN);
        if(!validCid(cid))
            continue;

        Entry entry;
        try {
            entry.shareSize = Util::toInt64(File(sizePath, File::READ, File::OPEN).read());
        } catch(const Exception&) {
            continue;
        }

        const string fetchPath = sizePath.substr(0, sizePath.size() - SIZE_EXT.size()) + FETCH_EXT;
        try {
            if(File::getSize(fetchPath) != -1)
                entry.fetchTime = static_cast<time_t>(
                    Util::toInt64(File(fetchPath, File::READ, File::OPEN).read()));
        } catch(const Exception&) { }

        {
            Lock l(dataCs);
            const auto old = entries.find(cid);
            if(old == entries.end() || entry.fetchTime > old->second.fetchTime)
                entries[cid] = entry;
        }
        migrated.push_back(sizePath);
        migrated.push_back(fetchPath);
    }
    return migrated;
}
void loadStore() {
    try {
        readXml();
    } catch(const Exception&) { }

    const StringList migrated = migrateSidecars();
    if(migrated.empty())
        return;

    try {
        writeXml();
        for(const auto& path: migrated)
            File::deleteFile(path);
    } catch(const Exception&) {
        // Keep legacy files when centralized persistence fails.
    }
}
} // namespace
void load() {
    std::call_once(loadFlag, loadStore);
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
    try {
        writeXml();
    } catch(const Exception&) { }
}

} // namespace ListCacheStore

} // namespace dcpp
