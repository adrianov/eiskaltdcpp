/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ADC outbound chat, password, and INF identity updates.
 */

#include "stdinc.h"
#include "AdcHub.h"

#include "ClientManager.h"
#include "ConnectivityManager.h"
#include "CryptoManager.h"
#include "SearchManager.h"
#include "SettingsManager.h"
#include "ShareManager.h"
#include "StringTokenizer.h"
#include "ThrottleManager.h"
#include "UploadManager.h"
#include "UserCommand.h"
#include "Util.h"
#include "version.h"

namespace dcpp {

void AdcHub::hubMessage(const string& aMessage, bool thirdPerson) {
    if(state != STATE_NORMAL)
        return;
    AdcCommand c(AdcCommand::CMD_MSG, AdcCommand::TYPE_BROADCAST);
    c.addParam(aMessage);
    if(thirdPerson)
        c.addParam("ME", "1");
    send(c);
}

void AdcHub::privateMessage(const OnlineUser& user, const string& aMessage, bool thirdPerson) {
    if(state != STATE_NORMAL)
        return;
    AdcCommand c(AdcCommand::CMD_MSG, user.getIdentity().getSID(), AdcCommand::TYPE_ECHO);
    c.addParam(aMessage);
    if(thirdPerson)
        c.addParam("ME", "1");
    c.addParam("PM", getMySID());
    send(c);
}

void AdcHub::sendUserCmd(const UserCommand& command, const ParamMap& params) {
    if(state != STATE_NORMAL)
        return;
    string cmd = Util::formatParams(command.getCommand(), params, false);
    if(command.isChat()) {
        if(command.getTo().empty()) {
            hubMessage(cmd);
        } else {
            const string& to = command.getTo();
            Lock l(cs);
            for(auto& i: users) {
                if(i.second->getIdentity().getNick() == to) {
                    privateMessage(*i.second, cmd);
                    return;
                }
            }
        }
    } else {
        send(cmd);
    }
}

void AdcHub::password(const string& pwd) {
    if(state != STATE_VERIFY)
        return;
    if(!salt.empty()) {
        size_t saltBytes = salt.size() * 5 / 8;
        std::unique_ptr<uint8_t[]> buf(new uint8_t[saltBytes]);
        Encoder::fromBase32(salt.c_str(), &buf[0], saltBytes);
        TigerHash th;
        if(oldPassword) {
            CID cid = getMyIdentity().getUser()->getCID();
            th.update(cid.data(), CID::SIZE);
        }
        th.update(pwd.data(), pwd.length());
        th.update(&buf[0], saltBytes);
        send(AdcCommand(AdcCommand::CMD_PAS, AdcCommand::TYPE_HUB).addParam(Encoder::toBase32(th.finalize(), TigerHash::BYTES)));
        salt.clear();
    }
}

static void addParam(StringMap& lastInfoMap, AdcCommand& c, const string& var, const string& value) {
    auto i = lastInfoMap.find(var);

    if(i != lastInfoMap.end()) {
        if(i->second != value) {
            if(value.empty()) {
                lastInfoMap.erase(i);
            } else {
                i->second = value;
            }
            c.addParam(var, value);
        }
    } else if(!value.empty()) {
        lastInfoMap.emplace(var, value);
        c.addParam(var, value);
    }
}

void AdcHub::info(bool /*alwaysSend*/) {
    if(state != STATE_IDENTIFY && state != STATE_NORMAL)
        return;

    reloadSettings(false);

    AdcCommand c(AdcCommand::CMD_INF, AdcCommand::TYPE_BROADCAST);
    if (state == STATE_NORMAL) {
        updateCounts(false);
    }
    string app_name = string(APPNAME);
    string app_version = string(VERSIONSTRING);
    StringTokenizer<string> st(getClientId(), ' ');
    if(st.getTokens().size() == 2) {
        app_name = st.getTokens().at(0);
        app_version = st.getTokens().at(1);
    }

    addParam(lastInfoMap, c, "ID", ClientManager::getInstance()->getMyCID().toBase32());
    addParam(lastInfoMap, c, "PD", ClientManager::getInstance()->getMyPID().toBase32());
    addParam(lastInfoMap, c, "NI", getCurrentNick());
    addParam(lastInfoMap, c, "DE", getCurrentDescription());
    addParam(lastInfoMap, c, "SL", Util::toString(SETTING(SLOTS)));
    addParam(lastInfoMap, c, "FS", Util::toString(UploadManager::getInstance()->getFreeSlots()));
    addParam(lastInfoMap, c, "SS", ShareManager::getInstance()->getShareSizeString());
    addParam(lastInfoMap, c, "SF", Util::toString(ShareManager::getInstance()->getSharedFiles()));
    addParam(lastInfoMap, c, "EM", SETTING(EMAIL));
    addParam(lastInfoMap, c, "HN", Util::toString(counts.normal));
    addParam(lastInfoMap, c, "HR", Util::toString(counts.registered));
    addParam(lastInfoMap, c, "HO", Util::toString(counts.op));
    addParam(lastInfoMap, c, "AP", app_name);
    addParam(lastInfoMap, c, "VE", app_version);
    addParam(lastInfoMap, c, "AW", Util::getAway() ? "1" : Util::emptyString);
    int limit = ThrottleManager::getInstance()->getDownLimit();
    if (limit > 0 && BOOLSETTING(THROTTLE_ENABLE)) {
        addParam(lastInfoMap, c, "DS", Util::toString(limit * 1024));
    } else {
        addParam(lastInfoMap, c, "DS", Util::emptyString);
    }
    limit = ThrottleManager::getInstance()->getUpLimit();
    if (limit > 0 && BOOLSETTING(THROTTLE_ENABLE)) {
        addParam(lastInfoMap, c, "US", Util::toString(limit * 1024));
    } else {
        addParam(lastInfoMap, c, "US", Util::toString((long)(Util::toDouble(SETTING(UPLOAD_SPEED))*1024*1024/8)));
    }

    string su(SEGA_FEATURE);
    if(CryptoManager::getInstance()->TLSOk()) {
        su += "," + ADCS_FEATURE;
        auto &kp = CryptoManager::getInstance()->getKeyprint();
        addParam(lastInfoMap, c, "KP", "SHA256/" + Encoder::toBase32(&kp[0], kp.size()));
    }

    string i4;
    if (!getFavIp().empty())
        i4 = Util::normalizeIpv4(Socket::resolve(getFavIp()));
    if (i4.empty() && BOOLSETTING(NO_IP_OVERRIDE) && !SETTING(EXTERNAL_IP).empty())
        i4 = Util::normalizeIpv4(Socket::resolve(SETTING(EXTERNAL_IP)));
    // 0.0.0.0 asks the hub to fill in our real address.
    addParam(lastInfoMap, c, "I4", i4.empty() ? "0.0.0.0" : i4);

    if(isActive()) {
        addParam(lastInfoMap, c, "U4", SearchManager::getInstance()->getPort());
        su += "," + TCP4_FEATURE;
        su += "," + UDP4_FEATURE;
    } else {
        if (BOOLSETTING(ALLOW_NATT))
            su += "," + NAT0_FEATURE;
        else
            addParam(lastInfoMap, c, "I4", "");
        addParam(lastInfoMap, c, "U4", "");
    }

    addParam(lastInfoMap, c, "SU", su);

    if(!c.getParameters().empty()) {
        send(c);
    }
}

} // namespace dcpp
