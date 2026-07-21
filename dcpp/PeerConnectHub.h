/*
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "forward.h"
#include "typedefs.h"

namespace dcpp {

namespace PeerConnectHub {

enum HubOutcome {
    UNKNOWN = 0,
    SUCCESS = 1,
    FAILURE = 2
};

HubOutcome get(const UserPtr& user, const string& hub);
void rememberSuccess(const UserPtr& user, const string& hub);
void rememberFailure(const UserPtr& user, const string& hub);
/** Session list: hubs where $ConnectToMe timed out; skipped until all are listed. */
void noteConnectTimeout(const UserPtr& user, const string& hub);
bool isConnectTimeoutHub(const UserPtr& user, const string& hub);
void clearConnectTimeouts(const UserPtr& user);
/** Peer answered (slot-wait / inbound or associated download) — not unreachable. */
void notePeerReached(const UserPtr& user);
bool wasPeerReached(const UserPtr& user);
/** Session: dropped as silent/unreachable — ShareIndex must not auto-reattach. */
void noteUnreachablePeer(const UserPtr& user);
bool isUnreachablePeer(const UserPtr& user);
bool isUnreachableCid(const CID& cid);
void clearUnreachablePeer(const UserPtr& user);
/** Clear timeout skips and reached flag (CQI gone). Keeps unreachable session flag. */
void clearPeerSession(const UserPtr& user);
void sortSources(HintedUserList& sources);
void load();
void save();

} // namespace PeerConnectHub

} // namespace dcpp
