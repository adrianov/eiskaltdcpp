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

/** Inetra Peers network clients (peers.cn.ru): Pikachu / Pikachundr. */
bool isPikachuFamily(const string& c) {
    if(c.empty())
        return false;
    string l = Text::toLower(c);
    return Util::findSubString(l, "pikachundr") != string::npos
        || Util::findSubString(l, "pikachu") != string::npos;
}

} // namespace

/**
 * Peers.cn.ru clone farm: tens of thousands of Pikachundr accounts advertise the
 * same ~137 GiB share and random nicks; file lists are empty. Real Pikachu 1.0.7
 * peers use distinct share sizes (e.g. ~24 GiB) and remain viable.
 */
bool isPikachuGhost(const Identity& id) {
    if(!isPikachuFamily(clientTag(id)))
        return false;
    const int64_t share = id.getBytesShared();
    if(share <= 0)
        return true;
    // Dominant swarm size observed on peers.cn.ru (147102629888 bytes).
    return share == 147102629888LL;
}

bool isViablePeer(const OnlineUser& ou) {
    const Identity& id = ou.getIdentity();
    const UserPtr& u = ou.getUser();

    if(u->isSet(User::BOT) || isPikachuGhost(id))
        return false;

    if(hasClientTag(clientTag(id)))
        return true;

    // Tagless MyINFO (e.g. hmn.pp.ru) still carries connection type and share size.
    if(!id.getConnection().empty() || id.getBytesShared() > 0)
        return true;

    return false;
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
