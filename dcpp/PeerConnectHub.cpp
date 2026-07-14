/*
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "PeerConnectHub.h"

#include "HintedUser.h"

namespace dcpp {

namespace PeerConnectHub {

namespace {

FastCriticalSection hubCs;
unordered_map<CID, unordered_map<string, HubOutcome>> hubOutcomes;

void remember(const UserPtr& user, const string& hub, HubOutcome outcome) {
    if(!user || hub.empty())
        return;
    FastLock l(hubCs);
    hubOutcomes[user->getCID()][hub] = outcome;
}

} // namespace

HubOutcome get(const UserPtr& user, const string& hub) {
    if(!user || hub.empty())
        return UNKNOWN;
    FastLock l(hubCs);
    auto ci = hubOutcomes.find(user->getCID());
    if(ci == hubOutcomes.end())
        return UNKNOWN;
    auto hi = ci->second.find(hub);
    return hi != ci->second.end() ? hi->second : UNKNOWN;
}

void rememberSuccess(const UserPtr& user, const string& hub) {
    remember(user, hub, SUCCESS);
}

void rememberFailure(const UserPtr& user, const string& hub) {
    remember(user, hub, FAILURE);
}

int preference(const UserPtr& user, const string& hub) {
    switch(get(user, hub)) {
    case SUCCESS: return 0;
    case FAILURE: return 2;
    default: return 1;
    }
}

void sortSources(HintedUserList& sources) {
    if(sources.size() < 2)
        return;
    stable_sort(sources.begin(), sources.end(), [](const HintedUser& a, const HintedUser& b) {
        return preference(a.user, a.hint) < preference(b.user, b.hint);
    });
}

} // namespace PeerConnectHub

} // namespace dcpp
