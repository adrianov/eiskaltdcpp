/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "FavoriteManager.h"

#include "File.h"
#include "FilteredFile.h"
#include "HttpConnection.h"
#include "SimpleXML.h"
#include "SimpleXMLReader.h"
#include "Streams.h"
#include "BZUtils.h"
#include "Util.h"

namespace dcpp {

void FavoriteManager::abortHttp() {
    if(!c)
        return;
    c->removeListener(this);
    delete c;
    c = nullptr;
    running = false;
}

class XmlListLoader : public SimpleXMLReader::CallBack {
public:
    XmlListLoader(HubEntryList& lst) : publicHubs(lst) { }
    void startTag(const string& name, StringPairList& attribs, bool) {
        if(name == "Hub") {
            const string& name = getAttrib(attribs, "Name", 0);
            const string& server = getAttrib(attribs, "Address", 1);
            const string& description = getAttrib(attribs, "Description", 2);
            const string& users = getAttrib(attribs, "Users", 3);
            const string& country = getAttrib(attribs, "Country", 4);
            const string& shared = getAttrib(attribs, "Shared", 5);
            const string& minShare = getAttrib(attribs, "Minshare", 5);
            const string& minSlots = getAttrib(attribs, "Minslots", 5);
            const string& maxHubs = getAttrib(attribs, "Maxhubs", 5);
            const string& maxUsers = getAttrib(attribs, "Maxusers", 5);
            const string& reliability = getAttrib(attribs, "Reliability", 5);
            const string& rating = getAttrib(attribs, "Rating", 5);
            publicHubs.emplace_back(name, server, description, users, country, shared, minShare, minSlots, maxHubs, maxUsers, reliability, rating);
        }
    }

private:
    HubEntryList& publicHubs;
};

bool FavoriteManager::onHttpFinished(bool fromHttp) noexcept {
    MemoryInputStream mis(downloadBuf);
    bool success = true;

    Lock l(cs);
    HubEntryList& list = publicListMatrix[publicListServer];
    list.clear();

    try {
        XmlListLoader loader(list);

        if((listType == TYPE_BZIP2) && (!downloadBuf.empty())) {
            FilteredInputStream<UnBZFilter, false> f(&mis);
            SimpleXMLReader(&loader).parse(f);
        } else {
            SimpleXMLReader(&loader).parse(mis);
        }
    } catch(const Exception&) {
        success = false;
        fire(FavoriteManagerListener::Corrupted(), fromHttp ? publicListServer : Util::emptyString);
    }

    if(fromHttp) {
        try {
            File f(Util::getHubListsPath() + Util::validateFileName(publicListServer, "/"), File::WRITE, File::CREATE | File::TRUNCATE);
            f.write(downloadBuf);
            f.close();
        } catch(const FileException&) { }
    }

    downloadBuf = Util::emptyString;

    return success;
}

} // namespace dcpp
