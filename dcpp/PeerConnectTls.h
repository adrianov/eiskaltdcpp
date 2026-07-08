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

namespace dcpp {

class ConnectionQueueItem;

namespace PeerConnectTls {

enum SecureMode {
    AUTO = -1,
    PLAIN = 0,
    TLS = 1
};

bool tlsAvailable();
bool peerSupportsTls(const UserPtr& user);
bool learnedTlsRequired(const UserPtr& user);
void rememberTlsRequired(const UserPtr& user);
bool resolveSecure(int mode, const UserPtr& user);
bool rejectPeer(const UserPtr& user);
bool isTlsMismatch(const string& err);
void scheduleRetry(ConnectionQueueItem* cqi, bool wasSecure, bool protocolError, int connectPhase, const string& err);

} // namespace PeerConnectTls

} // namespace dcpp
