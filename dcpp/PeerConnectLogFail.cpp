/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "PeerConnectLog.h"

#include "ClientManager.h"
#include "LogManager.h"
#include "UserConnection.h"
#include "format.h"

#include <unordered_map>

namespace dcpp {

namespace {

void logMsg(const string& msg) {
    LogManager::getInstance()->message(string("[Connect] ") + msg);
}

string linkInfo(const UserConnection* uc) {
    const string proto = uc->isSet(UserConnection::FLAG_NMDC) ? "NMDC" : "ADC";
    const string tls = uc->isSecure() ?
            (uc->isTrusted() ? "TLS" : "TLS untrusted") : "plain";
    string dir = "?";
    if(uc->isSet(UserConnection::FLAG_DOWNLOAD))
        dir = "download";
    else if(uc->isSet(UserConnection::FLAG_UPLOAD))
        dir = "upload";
    const string inc = uc->isSet(UserConnection::FLAG_INCOMING) ? "incoming" : "outgoing";
    string remote = uc->getRemoteIp();
    if(!uc->getPort().empty())
        remote += ":" + uc->getPort();
    string ret = str(F_("%1% %2% %3% %4% from %5%") % proto % tls % dir % inc % remote);
    if(uc->isSecure() && !uc->getCipherName().empty())
        ret += ", cipher=" + uc->getCipherName();
    return ret;
}

string directionDetail(const UserConnection* uc) {
    if(!uc || uc->getState() != UserConnection::STATE_DIRECTION)
        return Util::emptyString;
    string ret = str(F_("sent $Direction %1% %2%") % uc->getDirectionString() % uc->getNumber());
    if(!uc->getPeerDirection().empty())
        ret += str(F_(", peer $Direction %1% %2%") % uc->getPeerDirection() % uc->getPeerDirectionNum());
    else
        ret += _(", no peer $Direction received");
    return ret;
}

/** One log line per key per minute; returns false when this hit was suppressed. */
bool allowRateLimitedLog(unordered_map<string, pair<uint64_t, unsigned>>& log,
        CriticalSection& cs, const string& key, unsigned& suppressed) {
    constexpr uint64_t kInterval = 60 * 1000;
    Lock l(cs);
    const uint64_t tick = GET_TICK();
    auto& e = log[key];
    if(e.first && tick < e.first + kInterval) {
        ++e.second;
        return false;
    }
    suppressed = e.second;
    e = { tick, 0 };
    if(log.size() > 256) {
        for(auto it = log.begin(); it != log.end(); ) {
            if(tick >= it->second.first + kInterval)
                it = log.erase(it);
            else
                ++it;
        }
    }
    return true;
}

} // namespace

namespace PeerConnectLog {

void skip(const string& target, const string& hub, const string& reason) {
    static CriticalSection skipCs;
    static unordered_map<string, pair<uint64_t, unsigned>> skipLog;
    unsigned suppressed = 0;
    if(!allowRateLimitedLog(skipLog, skipCs, target + '\n' + hub + '\n' + reason, suppressed))
        return;
    string msg = str(F_("%1% on %2%: %3%") % target % hub % reason);
    if(suppressed)
        msg += str(F_(" (+%1% similar)") % suppressed);
    logMsg(msg);
}

void connectionReject(const UserConnection* uc, const string& reason) {
    if(!uc)
        return;

    // Peers that spam $ConnectToMe while an upload slot is held can reject hundreds of
    // times per minute; keep one line per user per minute with a suppressed count.
    static CriticalSection rejectCs;
    static unordered_map<string, pair<uint64_t, unsigned>> rejectLog;
    unsigned suppressed = 0;
    if(uc->getUser()) {
        const string key = uc->getUser()->getCID().toBase32();
        if(!allowRateLimitedLog(rejectLog, rejectCs, key, suppressed))
            return;
    }

    string msg = reason;
    if(suppressed)
        msg += str(F_(" (+%1% similar)") % suppressed);
    const string dirDetail = directionDetail(uc);
    if(!dirDetail.empty())
        msg += " [" + dirDetail + "]";
    if(!uc->getRemoteIp().empty())
        msg += " (" + linkInfo(uc) + ")";

    if(uc->getUser())
        logMsg(str(F_("%1% on %2%: %3%") % ClientManager::getInstance()->getNickOrCid(uc->getHintedUser()) % uc->getHubUrl() % msg));
    else
        logMsg(str(F_("Incoming from %1% rejected: %2%") % "?" % msg));
}

} // namespace PeerConnectLog

} // namespace dcpp
