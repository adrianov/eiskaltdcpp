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
#include "Client.h"

namespace dcpp {

uint64_t Client::search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, const StringList& aExtList, void* owner){
    dcdebug("Queue search %s\n", aString.c_str());

    if(getSearchBlocked())
        return 0;

    if(searchQueue.interval) {
        SearchCore s;
        s.fileType = aFileType;
        s.size     = aSize;
        s.query    = aString;
        s.sizeType = aSizeMode;
        s.token    = aToken;
        s.exts     = aExtList;
        s.owners.insert(owner);

        searchQueue.add(s);

        uint64_t now = GET_TICK();
        return searchQueue.getSearchTime(owner, now) - now;
    }
    search(aSizeMode, aSize, aFileType , aString, aToken, aExtList);
    return 0;
}

void Client::on(Second, uint64_t aTick) noexcept {
    if(state == STATE_DISCONNECTED && getAutoReconnect() && (aTick > (getLastActivity() + getReconnDelay() * 1000)) )
        connect();
    if(!searchQueue.interval || getSearchBlocked()) return;

    if(isConnected()) {
        SearchCore s;

        if(searchQueue.pop(s, aTick)) {
            search(s.sizeType, s.size, s.fileType , s.query, s.token, s.exts);
        }
    }
}

} // namespace dcpp
