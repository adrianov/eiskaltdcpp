/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "HubSearchDenied.h"

#include "Client.h"
#include "SearchQueue.h"
#include "TimerManager.h"
#include "Util.h"

namespace dcpp {

namespace {

bool unitAt(const string& message, string::size_type i, const char* unit) {
    return Util::findSubString(message, unit, i) == i;
}

/** Parse "64 secs", "1 min 30 secs", "2 hours" after hub "search … in …". */
uint32_t parseSearchRateLimitSeconds(const string& message) {
    if(message.empty() || Util::findSubString(message, "search") == string::npos)
        return 0;
    const string::size_type inPos = Util::findSubString(message, " in ");
    if(inPos == string::npos)
        return 0;

    uint32_t total = 0;
    string::size_type i = inPos + 4;
    for(int parts = 0; parts < 3 && i < message.size(); ++parts) {
        while(i < message.size() && !isdigit(static_cast<unsigned char>(message[i]))) {
            if(message[i] == '.' || message[i] == '!' || message[i] == '?')
                return total;
            ++i;
        }
        if(i >= message.size())
            break;
        string::size_type j = i;
        while(j < message.size() && isdigit(static_cast<unsigned char>(message[j])))
            ++j;
        if(j == i || j - i > 4)
            break;
        const int n = Util::toInt(message.substr(i, j - i));
        i = j;
        while(i < message.size() && isspace(static_cast<unsigned char>(message[i])))
            ++i;
        if(i >= message.size())
            break;

        uint32_t mult = 0;
        if(unitAt(message, i, "week"))
            mult = 7 * 24 * 3600;
        else if(unitAt(message, i, "day"))
            mult = 24 * 3600;
        else if(unitAt(message, i, "hour"))
            mult = 3600;
        else if(unitAt(message, i, "min"))
            mult = 60;
        else if(unitAt(message, i, "sec"))
            mult = 1;
        else
            break;

        total += static_cast<uint32_t>(n) * mult;
        while(i < message.size() && isalpha(static_cast<unsigned char>(message[i])))
            ++i;
    }
    if(total < 1 || total > 3600)
        return 0;
    return total;
}

} // namespace

bool isSearchDeniedHubText(const string& message) {
    if(message.empty())
        return false;
    if(Util::findSubString(message, "can't search") != string::npos)
        return true;
    if(Util::findSubString(message, "cannot search") != string::npos)
        return true;
    if(Util::findSubString(message, "can not search") != string::npos)
        return true;
    if(Util::findSubString(message, "максимальная длина поиска") != string::npos)
        return true;
    if(Util::findSubString(message, "maximum search length") != string::npos)
        return true;
    if(Util::findSubString(message, "max search length") != string::npos)
        return true;
    if(Util::findSubString(message, "search length") != string::npos &&
            Util::findSubString(message, "50") != string::npos)
        return true;
    if(Util::findSubString(message, "search") != string::npos &&
            Util::findSubString(message, "registered with class") != string::npos &&
            (Util::findSubString(message, "must") != string::npos ||
             Util::findSubString(message, "need") != string::npos ||
             Util::findSubString(message, "require") != string::npos ||
             Util::findSubString(message, "unless") != string::npos ||
             Util::findSubString(message, "denied") != string::npos ||
             Util::findSubString(message, "not allowed") != string::npos ||
             Util::findSubString(message, "unable") != string::npos))
        return true;
    return false;
}

void noteSearchDenied(Client& client, const string& message) {
    if(client.getSearchBlocked() || !isSearchDeniedHubText(message))
        return;
    client.setSearchBlocked(true);
    client.clearSearchQueue();
}

void noteSearchRateLimit(SearchQueue& queue, const string& message) {
    const uint32_t seconds = parseSearchRateLimitSeconds(message);
    if(!seconds)
        return;
    queue.delayNext(seconds, GET_TICK());
}

} // namespace dcpp
