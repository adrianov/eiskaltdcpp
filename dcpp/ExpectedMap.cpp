/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2019 Boris Pek <tehnick-8@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "ExpectedMap.h"

#include "Util.h"

namespace dcpp {

void ExpectedMap::add(const string& utf8Nick, const string& aMyNick, const string& aHubUrl) {
    if(utf8Nick.empty())
        return;
    Lock l(cs);
    expectedConnections[utf8Nick].emplace_back(aMyNick, aHubUrl);
}

StringPair ExpectedMap::pop(ExpectMap::iterator i) {
    StringPair tmp = i->second.front();
    i->second.erase(i->second.begin());
    if(i->second.empty())
        expectedConnections.erase(i);
    return tmp;
}

StringPair ExpectedMap::removeExact(const string& wireNick) {
    if(wireNick.empty())
        return make_pair(Util::emptyString, Util::emptyString);
    Lock l(cs);
    auto i = expectedConnections.find(wireNick);
    if(i != expectedConnections.end() && !i->second.empty())
        return pop(i);
    return make_pair(Util::emptyString, Util::emptyString);
}

StringPair ExpectedMap::removeIf(const function<bool(const string&, const string&)>& pred) {
    Lock l(cs);
    for(auto i = expectedConnections.begin(); i != expectedConnections.end(); ++i) {
        if(i->second.empty())
            continue;
        const string& hubUrl = i->second.front().second;
        if(pred(i->first, hubUrl))
            return pop(i);
    }
    return make_pair(Util::emptyString, Util::emptyString);
}

} // namespace dcpp
