/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2019 Boris Pek <tehnick-8@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <unordered_map>
#include <vector>

#include "CriticalSection.h"
#include "HintedUser.h"
#include "NonCopyable.h"

namespace dcpp {

using std::unordered_map;
using std::vector;

class ConnectionQueueItem : private NonCopyable {
public:
    typedef ConnectionQueueItem* Ptr;
    typedef vector<Ptr> List;
    typedef List::iterator Iter;

    enum State {
        CONNECTING,                 // Recently sent request to connect
        WAITING,                    // Waiting to send request to connect
        NO_DOWNLOAD_SLOTS,          // Not needed right now
        ACTIVE                      // In one up/downmanager
    };

    ConnectionQueueItem(const HintedUser& user, bool aDownload);

    GETSET(string, token, Token);
    GETSET(uint64_t, lastAttempt, LastAttempt);
    GETSET(int, errors, Errors); // Number of connection errors, or -1 after a protocol error
    GETSET(int, connectAttempts, ConnectAttempts);
    GETSET(int, slotWaits, SlotWaits);
    GETSET(State, state, State);
    GETSET(bool, download, Download);
    GETSET(int, secureMode, SecureMode);

    const HintedUser& getUser() const { return user; }
    void setHubHint(const string& hub) { user.hint = hub; }

private:
    HintedUser user;
};

class ExpectedMap {
public:
    void add(const string& aNick, const string& aMyNick, const string& aHubUrl) {
        Lock l(cs);
        expectedConnections[aNick].emplace_back(aMyNick, aHubUrl);
    }

    StringPair remove(const string& aNick) {
        Lock l(cs);
        auto i = expectedConnections.find(aNick);

        if(i == expectedConnections.end() || i->second.empty())
            return make_pair(Util::emptyString, Util::emptyString);

        StringPair tmp = i->second.front();
        i->second.erase(i->second.begin());
        if(i->second.empty())
            expectedConnections.erase(i);

        return tmp;
    }

private:
    /** Nick -> pending (myNick, hubUrl) for expected NMDC incoming connections */
    typedef vector<StringPair> ExpectList;
    typedef unordered_map<string, ExpectList> ExpectMap;
    ExpectMap expectedConnections;

    CriticalSection cs;
};

// Comparing with a user...
inline bool operator==(ConnectionQueueItem::Ptr ptr, const UserPtr& aUser) { return ptr->getUser() == aUser; }

} // namespace dcpp
