/*
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "PeerConnectTls.h"

#include "ConnectionManager.h"
#include "CryptoManager.h"
#include "SettingsManager.h"
#include "User.h"
#include "UserConnection.h"

namespace dcpp {

namespace PeerConnectTls {

bool tlsAvailable() {
    return CryptoManager::getInstance()->TLSOk();
}

bool peerSupportsTls(const UserPtr& user) {
    return user && user->isSet(User::TLS);
}

bool resolveSecure(int mode, const UserPtr& user) {
    if(!tlsAvailable())
        return false;
    if(mode == TLS)
        return true;
    if(mode == PLAIN)
        return false;
    if(BOOLSETTING(REQUIRE_TLS))
        return true;
    return peerSupportsTls(user);
}

bool rejectPeer(const UserPtr& user) {
    return BOOLSETTING(REQUIRE_TLS) && tlsAvailable() && user && user->isSet(User::NMDC) && !peerSupportsTls(user);
}

void scheduleRetry(ConnectionQueueItem* cqi, bool wasSecure, bool protocolError, int connectPhase) {
    if(!cqi || protocolError || connectPhase != UserConnection::STATE_CONNECT)
        return;

    const int mode = cqi->getSecureMode();
    if(wasSecure && mode != PLAIN && !BOOLSETTING(REQUIRE_TLS))
        cqi->setSecureMode(PLAIN);
    else if(!wasSecure && peerSupportsTls(cqi->getUser().user) && mode != TLS)
        cqi->setSecureMode(TLS);
}

} // namespace PeerConnectTls

} // namespace dcpp
