/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "PeerConnectFilter.h"

#include "OnlineUser.h"
#include "Util.h"

namespace dcpp {
namespace PeerConnectFilter {

namespace {

string clientTag(const Identity& id) {
    string c = id.getApplication();
    if(c.empty())
        c = id.get("TA");
    return c;
}

bool hasClientTag(const string& c) {
    return !c.empty() && c != "?";
}

bool isRevConnectFamily(const string& c) {
    if(c.empty())
        return false;
    string l = Text::toLower(c);
    return Util::findSubString(l, "strgdc") != string::npos
        || Util::findSubString(l, "strongdc") != string::npos
        || Util::findSubString(l, "flylink") != string::npos
        || Util::findSubString(l, "apexdc") != string::npos;
}

} // namespace

bool isViablePeer(const OnlineUser& ou) {
    const Identity& id = ou.getIdentity();
    const UserPtr& u = ou.getUser();

    if(hasClientTag(clientTag(id)))
        return true;

    if(u->isSet(User::BOT))
        return false;

    // Some hubs (e.g. Flylink-based) omit client tags from broadcast MyINFO but still
    // send connection speed and share size for real users.
    return !id.getConnection().empty() || id.getBytesShared() > 0;
}

bool prefersRevConnect(const OnlineUser& ou) {
    if(!ou.getUser()->isSet(User::PASSIVE))
        return false;
    return isRevConnectFamily(clientTag(ou.getIdentity()));
}

bool shouldGiveUp(int errors) {
    return errors >= MAX_CONNECT_ERRORS;
}

bool shouldGiveUpSlotWait(int slotWaits) {
    return slotWaits >= MAX_SLOT_WAITS;
}

int connectBackoffMs(int errors) {
    if(errors <= 0)
        return 60 * 1000;
    return 60 * 1000 * min(errors, MAX_CONNECT_ERRORS);
}

int slotWaitBackoffMs(int slotWaits) {
    if(slotWaits <= 0)
        return 0;
    return min(SLOT_WAIT_BASE_MS * slotWaits, 30 * 60 * 1000);
}

int queueBackoffMs(int errors, int slotWaits) {
    int ms = connectBackoffMs(errors);
    const int slotMs = slotWaitBackoffMs(slotWaits);
    return slotMs > ms ? slotMs : ms;
}

bool shouldLogTimeout(int errors) {
    return errors <= 1 || errors == 3 || errors >= MAX_CONNECT_ERRORS || (errors % 2) == 0;
}

bool shouldLogSlotWait(int slotWaits) {
    return slotWaits <= 1 || slotWaits == 3 || slotWaits >= MAX_SLOT_WAITS || (slotWaits % 3) == 0;
}

} // namespace PeerConnectFilter
} // namespace dcpp
