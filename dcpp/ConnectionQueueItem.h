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

inline bool isCcTag(char a, char b) {
    a = static_cast<char>(a | 0x20);
    b = static_cast<char>(b | 0x20);
    return a >= 'a' && a <= 'z' && b >= 'a' && b <= 'z';
}

/** Strip trailing [XX] some clients append in $MyNick (not hub disambiguation like fMad[BY]2). */
inline string stripWireCountryTag(const string& nick) {
    if(nick.size() >= 4 && nick[nick.size() - 4] == '[' && nick.back() == ']' &&
            isCcTag(nick[nick.size() - 3], nick[nick.size() - 2]))
        return nick.substr(0, nick.size() - 4);
    if(nick.size() >= 4 && nick[0] == '[' && nick[3] == ']' && isCcTag(nick[1], nick[2]))
        return nick.substr(4);
    return nick;
}

/** Match $MyNick wire form to hub user list nick (digit suffix, optional country tag). */
inline bool nickWireMatch(const string& wire, const string& hubNick) {
    if(wire == hubNick)
        return true;
    // Digit suffix (fMad[BY]2 vs fMad[BY]) — not for all-digit nicks like 123 vs 12346.
    if(wire.find_first_not_of("0123456789") != string::npos &&
            hubNick.size() > wire.size() && hubNick.compare(0, wire.size(), wire) == 0) {
        const string tail = hubNick.substr(wire.size());
        if(tail.find_first_not_of("0123456789") == string::npos)
            return true;
    }
    const string stripped = stripWireCountryTag(wire);
    return stripped != wire && stripped == hubNick;
}

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
    void setUser(const HintedUser& aUser) { user = aUser; }

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
        auto pop = [&](ExpectMap::iterator i) {
            StringPair tmp = i->second.front();
            i->second.erase(i->second.begin());
            if(i->second.empty())
                expectedConnections.erase(i);
            return tmp;
        };

        auto i = expectedConnections.find(aNick);
        if(i != expectedConnections.end() && !i->second.empty())
            return pop(i);

        string bestKey;
        size_t bestLen = 0;
        for(auto j = expectedConnections.begin(); j != expectedConnections.end(); ++j) {
            if(j->second.empty() || !nickWireMatch(aNick, j->first))
                continue;
            if(aNick == j->first)
                return pop(j);
            if(j->first.size() > bestLen) {
                bestKey = j->first;
                bestLen = j->first.size();
            }
        }
        if(!bestKey.empty()) {
            i = expectedConnections.find(bestKey);
            if(i != expectedConnections.end() && !i->second.empty())
                return pop(i);
        }
        return make_pair(Util::emptyString, Util::emptyString);
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
