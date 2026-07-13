/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "UploadRequestGuard.h"

#include "ClientManager.h"
#include "LogManager.h"
#include "TimerManager.h"
#include "User.h"
#include "format.h"

namespace dcpp {

namespace {

/** Same GET this often inside the window → ban (Tojishi flooded ~7/s). */
constexpr size_t MAX_IDENTICAL = 5;
constexpr uint64_t WINDOW_MS = 15 * 1000;
constexpr uint64_t BAN_MS = 10 * 60 * 1000;

} // namespace

string UploadRequestGuard::peerKey(const UserPtr& user, const string& remoteIp) const {
    // IP first: reconnect / nick change must not reset a flood ban.
    if(!remoteIp.empty())
        return "i:" + remoteIp;
    if(user)
        return "u:" + user->getCID().toBase32();
    return string();
}

string UploadRequestGuard::makeFingerprint(const string& type, const string& file,
                                           int64_t start, int64_t bytes) {
    return type + '\n' + file + '\n' + Util::toString(start) + '\n' + Util::toString(bytes);
}

void UploadRequestGuard::dropStale(Hit& hit, uint64_t now) const {
    while(!hit.ticks.empty() && hit.ticks.front() + WINDOW_MS < now)
        hit.ticks.pop_front();
}

bool UploadRequestGuard::allow(const UserPtr& user, const string& remoteIp,
                               const string& type, const string& file,
                               int64_t start, int64_t bytes) {
    const string key = peerKey(user, remoteIp);
    if(key.empty())
        return true;

    const uint64_t now = GET_TICK();
    const string fp = makeFingerprint(type, file, start, bytes);
    string banWho;
    string banType;
    bool logBan = false;

    {
        Lock l(cs);
        Hit& hit = peers[key];

        if(hit.bannedUntil > now) {
            hit.ticks.clear();
            return false;
        }
        if(hit.bannedUntil != 0) {
            hit.bannedUntil = 0;
            hit.loggedBan = false;
        }

        if(hit.fingerprint != fp) {
            hit.fingerprint = fp;
            hit.ticks.clear();
        }

        dropStale(hit, now);
        hit.ticks.push_back(now);

        if(hit.ticks.size() < MAX_IDENTICAL)
            return true;

        hit.bannedUntil = now + BAN_MS;
        hit.ticks.clear();
        if(!hit.loggedBan) {
            hit.loggedBan = true;
            logBan = true;
            banType = type;
            banWho = !remoteIp.empty() ? remoteIp : string();
        }
    }

    if(logBan) {
        if(user) {
            const string nick = ClientManager::getInstance()->getNickOrCid(
                user->getCID(), Util::emptyString);
            if(banWho.empty())
                banWho = nick;
            else if(nick != banWho)
                banWho = nick + " (" + banWho + ")";
        }
        if(banWho.empty())
            banWho = _("unknown");
        LogManager::getInstance()->message(str(F_(
            "Blocked upload flood from %1%: identical request repeated (%2%)")
            % banWho % banType));
    }
    return false;
}

void UploadRequestGuard::prune(uint64_t now) {
    Lock l(cs);
    for(auto i = peers.begin(); i != peers.end();) {
        Hit& hit = i->second;
        dropStale(hit, now);
        if(hit.ticks.empty() && hit.bannedUntil <= now)
            i = peers.erase(i);
        else
            ++i;
    }
}

} // namespace dcpp
