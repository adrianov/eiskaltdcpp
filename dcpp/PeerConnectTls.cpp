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

namespace {

FastCriticalSection tlsLearnCs;
unordered_set<CID> tlsRequired;

} // namespace

bool tlsAvailable() {
    return CryptoManager::getInstance()->TLSOk();
}

bool peerSupportsTls(const UserPtr& user) {
    return user && user->isSet(User::TLS);
}

bool learnedTlsRequired(const UserPtr& user) {
    if(!user)
        return false;
    FastLock l(tlsLearnCs);
    return tlsRequired.count(user->getCID()) != 0;
}

void rememberTlsRequired(const UserPtr& user) {
    if(!user)
        return;
    FastLock l(tlsLearnCs);
    tlsRequired.insert(user->getCID());
    user->setFlag(User::TLS);
}

bool resolveSecure(int mode, const UserPtr& user) {
    if(!tlsAvailable())
        return false;
    if(mode == TLS || learnedTlsRequired(user))
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

bool isTlsMismatch(const string& err) {
    if(err.empty())
        return false;
    const string lower = Text::toLower(err);
    return lower.find("secure connection") != string::npos || err.find("SSL") != string::npos;
}

void scheduleRetry(ConnectionQueueItem* cqi, bool wasSecure, bool protocolError, int connectPhase, const string& err) {
    if(!cqi)
        return;

    if(protocolError && isTlsMismatch(err)) {
        if(!wasSecure) {
            rememberTlsRequired(cqi->getUser().user);
            cqi->setSecureMode(TLS);
        } else if(!learnedTlsRequired(cqi->getUser().user) && !BOOLSETTING(REQUIRE_TLS)) {
            cqi->setSecureMode(PLAIN);
        }
        return;
    }

    if(protocolError || connectPhase != UserConnection::STATE_CONNECT)
        return;

    const int mode = cqi->getSecureMode();
    if(wasSecure && mode != PLAIN && !BOOLSETTING(REQUIRE_TLS) && !learnedTlsRequired(cqi->getUser().user))
        cqi->setSecureMode(PLAIN);
    else if(!wasSecure && (peerSupportsTls(cqi->getUser().user) || learnedTlsRequired(cqi->getUser().user)) && mode != TLS)
        cqi->setSecureMode(TLS);
}

} // namespace PeerConnectTls

} // namespace dcpp
