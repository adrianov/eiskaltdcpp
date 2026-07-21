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
#include "OnlineUser.h"
#include "PeerConnectFilter.h"
#include "format.h"

#include <unordered_map>

namespace dcpp {

namespace {

string userName(const HintedUser& user) {
    return ClientManager::getInstance()->getNickOrCid(user);
}

void logMsg(const string& msg) {
    LogManager::getInstance()->message(string("[Connect] ") + msg);
}

string hubList(const CID& cid, const string& hint) {
    StringList hubs = ClientManager::getInstance()->getHubs(cid, hint, false);
    if(hubs.empty())
        return hint.empty() ? "?" : hint;

    string ret;
    for(size_t i = 0; i < hubs.size(); ++i) {
        if(i)
            ret += ", ";
        ret += hubs[i];
    }
    return ret;
}

string modeStr(const UserPtr& u) {
    string m = u->isSet(User::PASSIVE) ? "passive" : "active";
    if(u->isSet(User::BOT))
        m += ",bot";
    if(u->isSet(User::TLS))
        m += ",TLS";
    if(u->isSet(User::NAT_TRAVERSAL))
        m += ",NAT";
    return m;
}

string clientStr(const Identity& id) {
    string c = id.getApplication();
    if(c.empty()) {
        c = id.get("TA");
        if(c.size() > 72)
            c = c.substr(0, 69) + "...";
    }
    return c.empty() ? "?" : c;
}

string profile(const OnlineUser& ou) {
    return str(F_("hub=%1%, %2%, client=%3%") % ou.getClient().getHubUrl() % modeStr(ou.getUser()) % clientStr(ou.getIdentity()));
}

string profile(const HintedUser& user) {
    OnlineUser* ou = ClientManager::getInstance()->findOnlineUser(user, false);
    const string hubs = hubList(user.user->getCID(), user.hint);
    if(!ou)
        return str(F_("hubs=[%1%], status=offline on hint") % hubs);
    return str(F_("hubs=[%1%], %2%") % hubs % profile(*ou));
}

string withProfile(const HintedUser& user, const string& msg) {
    return msg + " (" + profile(user) + ")";
}

} // namespace

namespace PeerConnectLog {

void queueAdded(const HintedUser& user) {
    logMsg(withProfile(user, str(F_("Queued connection to %1%") % userName(user))));
}

void queueStart(const HintedUser& user) {
    logMsg(withProfile(user, str(F_("Connecting to %1%") % userName(user))));
}

void queueTimeout(const HintedUser& user, int errors) {
    if(!PeerConnectFilter::shouldLogTimeout(errors))
        return;
    logMsg(withProfile(user, str(F_("Connection timeout for %1% (attempt %2%)") % userName(user) % errors)));
}

void queueUnreachable(const HintedUser& user) {
    logMsg(withProfile(user, str(F_("Peer %1% unreachable on all hubs — removed from download queue sources")
            % userName(user))));
}

void queueGiveUp(const HintedUser& user, int errors) {
    logMsg(withProfile(user, str(F_("Giving up connection to %1% after %2% failed attempts") % userName(user) % errors)));
}

void queueSlotWaitGiveUp(const HintedUser& user, int slotWaits) {
    logMsg(withProfile(user, str(F_("Giving up connection to %1% after %2% upload-slot waits") % userName(user) % slotWaits)));
}

void queueSlotWait(const HintedUser& user, int slotWaits, int backoffMin) {
    if(!PeerConnectFilter::shouldLogSlotWait(slotWaits))
        return;
    logMsg(withProfile(user, str(F_("Upload queue wait for %1%: backing off %2% min before retry") % userName(user) % backoffMin)));
}

void connected(const HintedUser& user, bool download) {
    if(!download && user.user) {
        // Upload connect churn (peer $ConnectToMe spam) can log hundreds of lines/min.
        static CriticalSection cs;
        static unordered_map<CID, pair<uint64_t, unsigned>> last;
        constexpr uint64_t kInterval = 60 * 1000;
        unsigned suppressed = 0;
        {
            Lock l(cs);
            const uint64_t tick = GET_TICK();
            auto& e = last[user.user->getCID()];
            if(e.first && tick < e.first + kInterval) {
                ++e.second;
                return;
            }
            suppressed = e.second;
            e = { tick, 0 };
        }
        string msg = str(F_("Connected to %1% (upload)") % userName(user));
        if(suppressed)
            msg += str(F_(" (+%1% similar)") % suppressed);
        logMsg(withProfile(user, msg));
        return;
    }
    logMsg(withProfile(user, str(F_("Connected to %1% (%2%)") % userName(user) % (download ? "download" : "upload"))));
}

void userOffline(const HintedUser& user) {
    logMsg(withProfile(user, str(F_("%1%: user not online on hub") % userName(user))));
}

void passiveWait(const HintedUser& user) {
    skip(userName(user), user.hint.empty() ? string("?") : user.hint,
            _("waiting for active user (both passive)"));
}

void passiveSkip(const HintedUser& user) {
    skip(userName(user), user.hint.empty() ? string("?") : user.hint,
            _("skipped passive-to-passive source"));
}

void fakeActiveRetry(const HintedUser& user) {
    logMsg(withProfile(user, str(F_("%1%: retrying as passive (active connect failed)") % userName(user))));
}

void cachedList(const HintedUser& user, const string& listFile) {
    logMsg(withProfile(user, str(F_("%1%: opened cached file list (%2%)") % userName(user) % listFile)));
}

void nmdcSend(const OnlineUser& target, const string& cmd, const string& detail) {
    logMsg(str(F_("%1%: sent %2% (%3%) [%4%]") % target.getIdentity().getNick() % cmd % detail % profile(target)));
}

void nmdcRecv(const OnlineUser& sender, const string& summary) {
    logMsg(str(F_("NMDC: %1% [%2%]") % summary % profile(sender)));
}

void nmdcRecv(const string& hub, const string& summary) {
    logMsg(str(F_("NMDC on %1%: %2%") % hub % summary));
}

void adcSend(const OnlineUser& target, const string& cmd, const string& detail) {
    logMsg(str(F_("%1%: sent %2% (%3%) [%4%]") % target.getIdentity().getNick() % cmd % detail % profile(target)));
}

void tcpOut(const string& user, const string& host, const string& port, bool secure, const string& proto) {
    logMsg(str(F_("%1%: TCP %2% to %3%:%4%") % user % proto % host % port) +
           (secure ? string(" TLS") : Util::emptyString));
}

void tcpFail(const string& user, const string& host, const string& port, const string& err) {
    logMsg(str(F_("%1%: TCP to %2%:%3% failed: %4%") % user % host % port % err));
}

void incomingReject(const string& nick, const string& reason) {
    logMsg(str(F_("Incoming from %1% rejected: %2%") % nick % reason));
}

} // namespace PeerConnectLog

} // namespace dcpp
