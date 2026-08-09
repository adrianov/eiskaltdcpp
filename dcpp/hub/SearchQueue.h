/*
 * Copyright (C) 2003-2006 RevConnect, http://www.revconnect.com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "MerkleTree.h"

namespace dcpp {

/** One hub search request. Manual tokens run before token "auto". */
struct SearchCore
{
    int32_t     sizeType;
    int64_t     size;
    int32_t     fileType;
    string      query;
    string      token;
    StringList  exts;
    std::unordered_set<void*>  owners;

    bool isAuto() const { return token == "auto"; }

    bool operator==(const SearchCore& rhs) const {
        return sizeType == rhs.sizeType &&
                size == rhs.size &&
                fileType == rhs.fileType &&
                query == rhs.query &&
                token == rhs.token;
    }
};

/**
 * Per-hub search schedule.
 * interval: minimum ms between sends (0 = no spacing; still one drain pass).
 * nextAllowed: earliest tick the next search may leave (raised by hub rate-limit text).
 * Queue order: manuals before autos; FIFO within each class → earliest run time by position.
 */
class SearchQueue
{
public:
    SearchQueue(uint64_t aInterval = 0)
        : interval(aInterval), nextAllowed(0)
    {
    }

    bool add(const SearchCore& s);
    bool pop(SearchCore& s, uint64_t now);
    void clear();
    /** Drop this owner from every queued item (stop / close / re-search). */
    bool cancelSearch(void* aOwner);
    /** Absolute tick when owner's first queued item may run; 0 if not queued. */
    uint64_t getSearchTime(void* aOwner, uint64_t now) const;
    /** Hub guard: wait at least `seconds` from `now`; raise interval floor. */
    void delayNext(uint32_t seconds, uint64_t now);

    uint64_t interval;     ///< ms; 0 = no minimum spacing
    uint64_t nextAllowed;  ///< tick; 0 = send as soon as connected/queued

private:
    void insertByPriority(const SearchCore& s);

    deque<SearchCore> searchQueue;
    mutable CriticalSection cs;
};

} // namespace dcpp
