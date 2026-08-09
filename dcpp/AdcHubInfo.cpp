/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ADC hub INF identity updates (outbound).
 */

#include "stdinc.h"
#include "AdcHub.h"

#include "ClientManager.h"
#include "CryptoManager.h"
#include "SearchManager.h"
#include "SettingsManager.h"
#include "ShareManager.h"
#include "StringTokenizer.h"
#include "ThrottleManager.h"
#include "UploadManager.h"
#include "Util.h"
#include "version.h"

namespace dcpp {

void AdcHub::info(bool /*alwaysSend*/) {
    if(state != STATE_IDENTIFY && state != STATE_NORMAL)
        return;

    reloadSettings(false);

    AdcCommand c(AdcCommand::CMD_INF, AdcCommand::TYPE_BROADCAST);
    if(state == STATE_NORMAL)
        updateCounts(false);

    AdcSelfInfo::Snap s;
    s.cid = ClientManager::getInstance()->getMyCID().toBase32();
    s.pid = ClientManager::getInstance()->getMyPID().toBase32();
    s.nick = getCurrentNick();
    s.description = getCurrentDescription();
    s.slotCount = Util::toString(SETTING(SLOTS));
    s.freeSlots = Util::toString(UploadManager::getInstance()->getFreeSlots());
    s.shareSize = ShareManager::getInstance()->getShareSizeString();
    s.shareFiles = Util::toString(ShareManager::getInstance()->getSharedFiles());
    s.email = SETTING(EMAIL);
    s.hubsNormal = Util::toString(counts.normal);
    s.hubsReg = Util::toString(counts.registered);
    s.hubsOp = Util::toString(counts.op);
    s.app = APPNAME;
    s.version = VERSIONSTRING;
    {
        StringTokenizer<string> st(getClientId(), ' ');
        if(st.getTokens().size() == 2) {
            s.app = st.getTokens().at(0);
            s.version = st.getTokens().at(1);
        }
    }
    s.away = Util::getAway() ? "1" : Util::emptyString;
    s.locale = SETTING(LANGUAGE);
    {
        string::size_type us = s.locale.find('_');
        if(us != string::npos)
            s.locale[us] = '-';
    }

    int limit = ThrottleManager::getInstance()->getDownLimit();
    if(limit > 0 && BOOLSETTING(THROTTLE_ENABLE))
        s.downLimit = Util::toString(limit * 1024);
    limit = ThrottleManager::getInstance()->getUpLimit();
    if(limit > 0 && BOOLSETTING(THROTTLE_ENABLE))
        s.upLimit = Util::toString(limit * 1024);
    else
        s.upLimit = Util::toString((long)(Util::toDouble(SETTING(UPLOAD_SPEED)) * 1024 * 1024 / 8));

    s.sega = SEGA_FEATURE;
    s.adcs = ADCS_FEATURE;
    s.tcp4 = TCP4_FEATURE;
    s.udp4 = UDP4_FEATURE;
    s.nat0 = NAT0_FEATURE;
    s.tls = CryptoManager::getInstance()->TLSOk();
    if(s.tls) {
        auto& kp = CryptoManager::getInstance()->getKeyprint();
        s.keyprint = "SHA256/" + Encoder::toBase32(&kp[0], kp.size());
    }

    string i4;
    if(!getFavIp().empty())
        i4 = Util::normalizeIpv4(Socket::resolve(getFavIp()));
    if(i4.empty() && BOOLSETTING(NO_IP_OVERRIDE) && !SETTING(EXTERNAL_IP).empty())
        i4 = Util::normalizeIpv4(Socket::resolve(SETTING(EXTERNAL_IP)));
    s.ip = i4.empty() ? "0.0.0.0" : i4;
    s.active = isActive();
    s.allowNatt = BOOLSETTING(ALLOW_NATT);
    if(s.active)
        s.udpPort = SearchManager::getInstance()->getPort();

    selfInfo.write(c, s);
    if(!c.getParameters().empty())
        send(c);
}

} // namespace dcpp
