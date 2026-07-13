/*
 * Copyright (C) 2003-2006 RevConnect, http://www.revconnect.com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "SearchQueue.h"

namespace dcpp {

void SearchQueue::clear() {
    Lock l(cs);
    searchQueue.clear();
}

void SearchQueue::insertByPriority(const SearchCore& s) {
    if(s.isAuto()) {
        searchQueue.push_back(s);
        return;
    }
    for(auto i = searchQueue.begin(); i != searchQueue.end(); ++i) {
        if(i->isAuto()) {
            searchQueue.insert(i, s);
            return;
        }
    }
    searchQueue.push_back(s);
}

bool SearchQueue::add(const SearchCore& s) {
    dcassert(s.owners.size() == 1);
    Lock l(cs);

    for(auto i = searchQueue.begin(); i != searchQueue.end(); ++i) {
        if(!(*i == s))
            continue;
        void* aOwner = *s.owners.begin();
        i->owners.insert(aOwner);
        // Promote auto → manual: re-place before other autos.
        if(!s.isAuto() && i->isAuto()) {
            SearchCore promoted = *i;
            promoted.token = s.token;
            searchQueue.erase(i);
            insertByPriority(promoted);
        }
        return false;
    }

    insertByPriority(s);
    return true;
}

bool SearchQueue::pop(SearchCore& s, uint64_t now) {
    Lock l(cs);
    if(searchQueue.empty())
        return false;
    if(nextAllowed != 0 && now < nextAllowed)
        return false;

    s = searchQueue.front();
    searchQueue.pop_front();
    nextAllowed = interval ? (now + interval) : now;
    return true;
}

uint64_t SearchQueue::getSearchTime(void* aOwner, uint64_t now) const {
    if(!aOwner)
        return 0;

    Lock l(cs);
    uint64_t t = (nextAllowed != 0 && nextAllowed > now) ? nextAllowed : now;
    const uint64_t step = interval ? interval : 0;

    for(const auto& item : searchQueue) {
        if(item.owners.find(aOwner) != item.owners.end())
            return t;
        t += step;
    }
    return 0;
}

bool SearchQueue::cancelSearch(void* aOwner) {
    dcassert(aOwner);
    Lock l(cs);
    bool removed = false;
    for(auto i = searchQueue.begin(); i != searchQueue.end(); ) {
        const auto j = i->owners.find(aOwner);
        if(j == i->owners.end()) {
            ++i;
            continue;
        }
        i->owners.erase(j);
        removed = true;
        if(i->owners.empty())
            i = searchQueue.erase(i);
        else
            ++i;
    }
    return removed;
}

void SearchQueue::delayNext(uint32_t seconds, uint64_t now) {
    if(!seconds)
        return;

    const uint64_t waitMs = (uint64_t)seconds * 1000;
    // Same +1s fudge as Client::setSearchInterval.
    const uint64_t minInterval = (uint64_t)(seconds + min(seconds, (uint32_t)1)) * 1000;
    if(interval < minInterval)
        interval = minInterval;

    const uint64_t until = now + waitMs;
    if(nextAllowed < until)
        nextAllowed = until;
}

} // namespace dcpp
