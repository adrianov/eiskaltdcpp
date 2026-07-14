/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2019 Boris Pek <tehnick-8@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include "CriticalSection.h"
#include "typedefs.h"

namespace dcpp {

using std::function;
using std::unordered_map;
using std::vector;

/**
 * Pending NMDC incoming peers after $ConnectToMe.
 * Keys are UTF-8 nicks (hub identity); wire $MyNick may use hub encoding.
 */
class ExpectedMap {
public:
    void add(const string& utf8Nick, const string& aMyNick, const string& aHubUrl);

    /** Exact wire-byte match (ASCII / same encoding as stored legacy keys). */
    StringPair removeExact(const string& wireNick);

    /**
     * Pop the first expect entry for which pred(utf8Key, hubUrl) is true.
     * Used for encoding-aware matching of Cyrillic (and similar) nicks.
     */
    StringPair removeIf(const function<bool(const string& utf8Nick, const string& hubUrl)>& pred);

    /** Drop pending expects for utf8Nick on every hub except keepHubUrl. */
    void clearOtherHubs(const string& utf8Nick, const string& keepHubUrl);

private:
    typedef vector<StringPair> ExpectList;
    typedef unordered_map<string, ExpectList> ExpectMap;
    ExpectMap expectedConnections;
    CriticalSection cs;

    StringPair pop(ExpectMap::iterator i);
};

} // namespace dcpp
