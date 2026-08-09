/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "SelfInfo.h"
#include "../Util.h"

namespace dcpp {

void AdcSelfInfo::put(AdcCommand& c, const string& key, const string& value) {
    auto i = last.find(key);
    if(i != last.end()) {
        if(i->second != value) {
            if(value.empty())
                last.erase(i);
            else
                i->second = value;
            c.addParam(key, value);
        }
    } else if(!value.empty()) {
        last.emplace(key, value);
        c.addParam(key, value);
    }
}

void AdcSelfInfo::write(AdcCommand& c, const Snap& s) {
    put(c, "ID", s.cid);
    put(c, "PD", s.pid);
    put(c, "NI", s.nick);
    put(c, "DE", s.description);
    put(c, "SL", s.slotCount);
    put(c, "FS", s.freeSlots);
    put(c, "SS", s.shareSize);
    put(c, "SF", s.shareFiles);
    put(c, "EM", s.email);
    put(c, "HN", s.hubsNormal);
    put(c, "HR", s.hubsReg);
    put(c, "HO", s.hubsOp);
    put(c, "AP", s.app);
    put(c, "VE", s.version);
    put(c, "AW", s.away);
    put(c, "LC", s.locale);
    put(c, "DS", s.downLimit);
    put(c, "US", s.upLimit);

    string su = s.sega;
    if(s.tls) {
        su += "," + s.adcs;
        put(c, "KP", s.keyprint);
    } else {
        put(c, "KP", Util::emptyString);
    }

    if(s.active) {
        put(c, "I4", s.ip);
        put(c, "U4", s.udpPort);
        su += "," + s.tcp4 + "," + s.udp4;
    } else {
        put(c, "I4", s.allowNatt ? s.ip : Util::emptyString);
        put(c, "U4", Util::emptyString);
        if(s.allowNatt)
            su += "," + s.nat0;
    }

    put(c, "SU", su);
}

} // namespace dcpp
