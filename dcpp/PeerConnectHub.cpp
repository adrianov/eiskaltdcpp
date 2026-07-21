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

#include "Encoder.h"
#include "File.h"
#include "HintedUser.h"
#include "SimpleXML.h"
#include "Util.h"

#include <mutex>

namespace dcpp {

namespace PeerConnectHub {

namespace {

FastCriticalSection hubCs;
unordered_map<CID, unordered_map<string, HubOutcome>> hubOutcomes;
bool dirty = false;
std::once_flag loadFlag;
enum { MAX_USERS = 2048 };

string storeFile() {
    return Util::getPath(Util::PATH_USER_CONFIG) + "PeerConnectHub.xml";
}

bool validCid(const string& base32) {
    return base32.size() == 39 && Encoder::isBase32(base32);
}

void ensureLoaded() {
    std::call_once(loadFlag, [] {
        if(File::getSize(storeFile()) == -1)
            return;

        try {
            SimpleXML xml;
            xml.fromXML(File(storeFile(), File::READ, File::OPEN).read());
            if(!xml.findChild("PeerConnectHub"))
                return;

            xml.stepIn();
            FastLock l(hubCs);
            while(xml.findChild("Hub")) {
                const string cidStr = xml.getChildAttrib("CID");
                const string hub = xml.getChildAttrib("Url");
                const string outcome = xml.getChildAttrib("Outcome");
                if(!validCid(cidStr) || hub.empty())
                    continue;

                HubOutcome value = UNKNOWN;
                if(outcome == "1")
                    value = SUCCESS;
                else if(outcome == "2")
                    value = FAILURE;
                else
                    continue;

                const CID cid(cidStr);
                if(hubOutcomes.size() >= MAX_USERS && hubOutcomes.find(cid) == hubOutcomes.end())
                    continue;
                hubOutcomes[cid][hub] = value;
            }
        } catch(const Exception&) { }
    });
}

void remember(const UserPtr& user, const string& hub, HubOutcome outcome) {
    if(!user || hub.empty())
        return;
    ensureLoaded();
    FastLock l(hubCs);
    const CID& cid = user->getCID();
    if(hubOutcomes.size() >= MAX_USERS && hubOutcomes.find(cid) == hubOutcomes.end())
        hubOutcomes.erase(hubOutcomes.begin());
    auto& slot = hubOutcomes[cid][hub];
    if(slot == outcome)
        return;
    slot = outcome;
    dirty = true;
}

int preference(const UserPtr& user, const string& hub) {
    switch(get(user, hub)) {
    case SUCCESS: return 0;
    case FAILURE: return 2;
    default: return 1;
    }
}

} // namespace

HubOutcome get(const UserPtr& user, const string& hub) {
    if(!user || hub.empty())
        return UNKNOWN;
    // load() at startup; avoid disk I/O on the connect hot path.
    FastLock l(hubCs);
    auto ci = hubOutcomes.find(user->getCID());
    if(ci == hubOutcomes.end())
        return UNKNOWN;
    auto hi = ci->second.find(hub);
    return hi != ci->second.end() ? hi->second : UNKNOWN;
}

void rememberSuccess(const UserPtr& user, const string& hub) {
    remember(user, hub, SUCCESS);
    clearConnectTimeouts(user);
}

void rememberFailure(const UserPtr& user, const string& hub) {
    remember(user, hub, FAILURE);
}

void sortSources(HintedUserList& sources) {
    if(sources.size() < 2)
        return;
    stable_sort(sources.begin(), sources.end(), [](const HintedUser& a, const HintedUser& b) {
        return preference(a.user, a.hint) < preference(b.user, b.hint);
    });
}

void load() {
    ensureLoaded();
}

void save() {
    ensureLoaded();
    unordered_map<CID, unordered_map<string, HubOutcome>> snapshot;
    {
        FastLock l(hubCs);
        if(!dirty)
            return;
        snapshot = hubOutcomes;
        dirty = false;
    }

    try {
        SimpleXML xml;
        xml.addTag("PeerConnectHub");
        xml.stepIn();
        for(const auto& peer: snapshot) {
            for(const auto& hub: peer.second) {
                if(hub.second == UNKNOWN)
                    continue;
                xml.addTag("Hub");
                xml.addChildAttrib("CID", peer.first.toBase32());
                xml.addChildAttrib("Url", hub.first);
                xml.addChildAttrib("Outcome", Util::toString(static_cast<int>(hub.second)));
            }
        }
        xml.stepOut();

        const string fName = storeFile();
        File out(fName + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
        BufferedOutputStream<false> f(&out);
        f.write(SimpleXML::utf8Header);
        xml.toXML(&f);
        f.flush();
        out.close();
        File::renameFile(fName + ".tmp", fName);
    } catch(const Exception&) {
        FastLock l(hubCs);
        dirty = true;
    }
}

} // namespace PeerConnectHub

} // namespace dcpp
