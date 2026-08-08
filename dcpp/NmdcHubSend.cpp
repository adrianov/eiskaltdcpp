/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"

#include "NmdcHub.h"

#include "ChatMessage.h"
#include "ConnectionManager.h"
#include "CryptoManager.h"
#include "PeerConnectLog.h"
#include "PeerConnectTls.h"
#include "SearchManager.h"
#include "ShareManager.h"
#include "ThrottleManager.h"
#include "UploadManager.h"
#include "UserCommand.h"

namespace dcpp {

#define checkstate() if(state != STATE_NORMAL) return

string NmdcHub::checkNick(const string& aNick) {
    string tmp = aNick;
    for(size_t i = 0; i < aNick.size(); ++i) {
        if(static_cast<uint8_t>(tmp[i]) <= 32 || tmp[i] == '|' || tmp[i] == '$' || tmp[i] == '<' || tmp[i] == '>') {
            tmp[i] = '_';
        }
    }
    return tmp;
}

void NmdcHub::connectToMe(const OnlineUser& aUser, int secureMode) {
    checkstate();
    dcdebug("NmdcHub::connectToMe %s\n", aUser.getIdentity().getNick().c_str());
    const string& utf8Nick = aUser.getIdentity().getNick();
    if(!ConnectionManager::getInstance()->allowOutgoingConnect(aUser.getUser())) {
        PeerConnectLog::skip(utf8Nick, getHubUrl(), _("recent $ConnectToMe still in flight"));
        return;
    }
    string nick = fromUtf8(utf8Nick);
    bool secure = allowSecureCtm() && PeerConnectTls::resolveSecureNmdc(secureMode, aUser);
    string port = secure ? ConnectionManager::getInstance()->getSecurePort() : ConnectionManager::getInstance()->getPort();
    if(port.empty()) {
        PeerConnectLog::skip(utf8Nick, getHubUrl(), _("not listening for connections"));
        return;
    }
    // Expect key is UTF-8 hub identity; wire $MyNick may use hub encoding.
    ConnectionManager::getInstance()->nmdcExpect(utf8Nick, getMyNick(), getHubUrl());
    ConnectionManager::getInstance()->noteOutgoingConnect(aUser.getUser());
    const string detail = getLocalIp() + ":" + port + (secure ? " TLS" : "");
    PeerConnectLog::nmdcSend(aUser, "$ConnectToMe", detail);
    send("$ConnectToMe " + nick + " " + getLocalIp() + ":" + port + (secure ? "S" : "") + "|");
}

void NmdcHub::revConnectToMe(const OnlineUser& aUser) {
    checkstate();
    dcdebug("NmdcHub::revConnectToMe %s\n", aUser.getIdentity().getNick().c_str());
    if(!ConnectionManager::getInstance()->allowOutgoingConnect(aUser.getUser())) {
        PeerConnectLog::skip(aUser.getIdentity().getNick(), getHubUrl(), _("recent $ConnectToMe still in flight"));
        return;
    }
    const string target = fromUtf8(aUser.getIdentity().getNick());
    ConnectionManager::getInstance()->noteOutgoingConnect(aUser.getUser());
    PeerConnectLog::nmdcSend(aUser, "$RevConnectToMe", fromUtf8(getMyNick()));
    send("$RevConnectToMe " + fromUtf8(getMyNick()) + " " + target + "|");
}

void NmdcHub::hubMessage(const string& aMessage, bool thirdPerson) {
    checkstate();
    send(fromUtf8( "<" + getMyNick() + "> " + escape(thirdPerson ? "/me " + aMessage : aMessage) + "|" ) );
}

void NmdcHub::myInfo(bool alwaysSend) {
    checkstate();

    reloadSettings(false);

    char StatusMode = Identity::NORMAL;

    char modeChar = '?';
    if(SETTING(OUTGOING_CONNECTIONS) == SettingsManager::OUTGOING_SOCKS5)
        modeChar = '5';
    else if(isActive())
        modeChar = 'A';
    else
        modeChar = 'P';
    const int upLimit = ThrottleManager::getInstance()->getUpLimit();
    const string uploadSpeed = (upLimit > 0 && BOOLSETTING(THROTTLE_ENABLE))
        ? Util::toString(upLimit) + "KiB/s" : SETTING(UPLOAD_SPEED);
    if(Util::getAway())
        StatusMode |= Identity::AWAY;
    // TLS/NAT bits: only if hub $Supports TLS (YnHub rejects them otherwise).
    if(supportFlags & SUPPORTS_TLS) {
        if(BOOLSETTING(ALLOW_NATT) && !isActive())
            StatusMode |= Identity::NAT;
        if(CryptoManager::getInstance()->TLSOk())
            StatusMode |= Identity::TLS;
    }

    string desc = getCurrentDescription();
    if(BOOLSETTING(SHOW_FREE_SLOTS_DESC))
        desc.insert(0, "[" + Util::toString(UploadManager::getInstance()->getFreeSlots()) + "]");
    desc.erase(std::remove_if(desc.begin(), desc.end(), [](char c){ return c == '<' || c == '>'; }), desc.end());
    const string uMin = ((supportFlags & SUPPORTS_TLS) && SETTING(MIN_UPLOAD_SPEED) != 0)
        ? ",O:" + Util::toString(SETTING(MIN_UPLOAD_SPEED)) : Util::emptyString;
    const string myInfoA =
            "$MyINFO $ALL " + fromUtf8(getMyNick()) + " " +
            fromUtf8(escape(desc)) + " <" + getClientId() + ",M:" + modeChar + ",H:" + getCounts();
    const string myInfoB = ",S:" + Util::toString(SETTING(SLOTS));
    const string myInfoC = uMin +
            ">$ $" + uploadSpeed + StatusMode + "$" + fromUtf8(escape(SETTING(EMAIL))) + '$';
    const string myInfoD = ShareManager::getInstance()->getShareSizeString() + "$|";
    if(alwaysSend ||
            ((lastMyInfoA != myInfoA || lastMyInfoC != myInfoC) && lastUpdate + 2*60*1000 < GET_TICK())
            ||
            ((lastMyInfoB != myInfoB || lastMyInfoD != myInfoD) && lastUpdate + 15*60*1000 < GET_TICK())) {
        send(myInfoA + myInfoB + myInfoC + myInfoD);
        lastMyInfoA = myInfoA;
        lastMyInfoB = myInfoB;
        lastMyInfoC = myInfoC;
        lastMyInfoD = myInfoD;
        lastUpdate = GET_TICK();
    }
}

void NmdcHub::search(int aSizeType, int64_t aSize, int aFileType, const string& aString, const string&, const StringList&) {
    checkstate();
    const bool active = !passiveSearch();
    // Short TTH search ($SA / $SP) when the hub listed TTHS in $Supports.
    if((supportFlags & SUPPORTS_SEARCH_TTHS) && aFileType == SearchManager::TYPE_TTH) {
        if(active)
            send("$SA " + aString + ' ' + getLocalIp() + ':' + SearchManager::getInstance()->getPort() + '|');
        else
            send("$SP " + aString + ' ' + fromUtf8(getMyNick()) + '|');
        return;
    }
    char c1 = (aSizeType == SearchManager::SIZE_DONTCARE) ? 'F' : 'T';
    char c2 = (aSizeType == SearchManager::SIZE_ATLEAST) ? 'F' : 'T';
    // NMDC has no Audio & Video type; send Any and let the client filter by extensions.
    if(aFileType == SearchManager::TYPE_AUDIO_VIDEO)
        aFileType = SearchManager::TYPE_ANY;
    string tmp = ((aFileType == SearchManager::TYPE_TTH) ? "TTH:" + aString : fromUtf8(escape(aString)));
    string::size_type i;
    while((i = tmp.find(' ')) != string::npos) {
        tmp[i] = '$';
    }
    string tmp2;
    if(active) {
        tmp2 = getLocalIp() + ':' + SearchManager::getInstance()->getPort();
    } else {
        tmp2 = "Hub:" + fromUtf8(getMyNick());
    }
    send("$Search " + tmp2 + ' ' + c1 + '?' + c2 + '?' + Util::toString(aSize) + '?' + Util::toString(aFileType+1) + '?' + tmp + '|');
}

void NmdcHub::privateMessage(const string& nick, const string& message) {
    send("$To: " + fromUtf8(nick) + " From: " + fromUtf8(getMyNick()) + " $" + fromUtf8(escape("<" + getMyNick() + "> " + message)) + "|");
}

void NmdcHub::privateMessage(const OnlineUser& aUser, const string& aMessage, bool /*thirdPerson*/) {
    checkstate();

    privateMessage(aUser.getIdentity().getNick(), aMessage);
    // Emulate a returning message...
    Lock l(cs);
    auto ou = findUser(getMyNick());
    if(ou) {
        ChatMessage message = { aMessage, ou, &aUser, ou, false, 0 };
        fire(ClientListener::Message(), this, message);
    }
}

void NmdcHub::sendUserCmd(const UserCommand& command, const ParamMap& params) {
    checkstate();
    string cmd = Util::formatParams(command.getCommand(), params, false);
    if(command.isChat()) {
        if(command.getTo().empty()) {
            hubMessage(cmd);
        } else {
            privateMessage(command.getTo(), cmd);
        }
    } else {
        send(fromUtf8(cmd));
    }
}
} // namespace dcpp
