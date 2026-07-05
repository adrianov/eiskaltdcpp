/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
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

void NmdcHub::connectToMe(const OnlineUser& aUser) {
    checkstate();
    dcdebug("NmdcHub::connectToMe %s\n", aUser.getIdentity().getNick().c_str());
    string nick = fromUtf8(aUser.getIdentity().getNick());
    ConnectionManager::getInstance()->nmdcExpect(nick, getMyNick(), getHubUrl());
    bool secure = CryptoManager::getInstance()->TLSOk() && aUser.getUser()->isSet(User::TLS);
    string port = secure ? ConnectionManager::getInstance()->getSecurePort() : ConnectionManager::getInstance()->getPort();
    send("$ConnectToMe " + nick + " " + getLocalIp() + ":" + port + (secure ? "S" : "") + "|");
}

void NmdcHub::revConnectToMe(const OnlineUser& aUser) {
    checkstate();
    dcdebug("NmdcHub::revConnectToMe %s\n", aUser.getIdentity().getNick().c_str());
    send("$RevConnectToMe " + fromUtf8(getMyNick()) + " " + fromUtf8(aUser.getIdentity().getNick()) + "|");
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
    string uploadSpeed;
    int upLimit = ThrottleManager::getInstance()->getUpLimit();
    if (upLimit > 0 && BOOLSETTING(THROTTLE_ENABLE)) {
        uploadSpeed = Util::toString(upLimit) + " KiB/s";
    } else {
        uploadSpeed = SETTING(UPLOAD_SPEED);
    }
    if(Util::getAway()) {
        StatusMode |= Identity::AWAY;
    }
    if(BOOLSETTING(ALLOW_NATT) && !isActive()) {
        StatusMode |= Identity::NAT;
    }
    if (CryptoManager::getInstance()->TLSOk()) {
        StatusMode |= Identity::TLS;
    }

    bool gslotf = BOOLSETTING(SHOW_FREE_SLOTS_DESC);
    string gslot = "["+Util::toString(UploadManager::getInstance()->getFreeSlots())+"]";
    string uMin = (SETTING(MIN_UPLOAD_SPEED) == 0) ? Util::emptyString : ",O:" + Util::toString(SETTING(MIN_UPLOAD_SPEED));
    string myInfoA =
            "$MyINFO $ALL " + fromUtf8(getMyNick()) + " " +
            fromUtf8(escape((gslotf ? gslot :"")+getCurrentDescription())) + " <"+ getClientId().c_str() + ",M:" + modeChar + ",H:" + getCounts();
    string myInfoB = ",S:" + Util::toString(SETTING(SLOTS));
    string myInfoC = uMin +
            ">$ $" + uploadSpeed + StatusMode + "$" + fromUtf8(escape(SETTING(EMAIL))) + '$';
    string myInfoD = ShareManager::getInstance()->getShareSizeString() + "$|";
    // we always send A and C; however, B (slots) and D (share size) can frequently change so we delay them if needed
    if(alwaysSend ||
            ((lastMyInfoA != myInfoA || lastMyInfoC != myInfoC) && lastUpdate + 2*60*1000 < GET_TICK())
            ||
            ((lastMyInfoB != myInfoB || lastMyInfoD != myInfoD) && lastUpdate + 15*60*1000 < GET_TICK())) {
        dcdebug("MyInfo %s...\n", getMyNick().c_str());
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
    char c1 = (aSizeType == SearchManager::SIZE_DONTCARE) ? 'F' : 'T';
    char c2 = (aSizeType == SearchManager::SIZE_ATLEAST) ? 'F' : 'T';
    string tmp = ((aFileType == SearchManager::TYPE_TTH) ? "TTH:" + aString : fromUtf8(escape(aString)));
    string::size_type i;
    while((i = tmp.find(' ')) != string::npos) {
        tmp[i] = '$';
    }
    string tmp2;
    if(isActive() && !BOOLSETTING(SEARCH_PASSIVE)) {
        tmp2 = getLocalIp() + ':' + SearchManager::getInstance()->getPort();
    } else {
        tmp2 = "Hub:" + fromUtf8(getMyNick());
    }
    send("$Search " + tmp2 + ' ' + c1 + '?' + c2 + '?' + Util::toString(aSize) + '?' + Util::toString(aFileType+1) + '?' + tmp + '|');
}

string NmdcHub::validateMessage(string tmp, bool reverse) {
    string::size_type i = 0;

    if(reverse) {
        while( (i = tmp.find("&#36;", i)) != string::npos) {
            tmp.replace(i, 5, "$");
            i++;
        }
        i = 0;
        while( (i = tmp.find("&#124;", i)) != string::npos) {
            tmp.replace(i, 6, "|");
            i++;
        }
        i = 0;
        while( (i = tmp.find("&amp;", i)) != string::npos) {
            tmp.replace(i, 5, "&");
            i++;
        }
    } else {
        i = 0;
        while( (i = tmp.find("&amp;", i)) != string::npos) {
            tmp.replace(i, 1, "&amp;");
            i += 4;
        }
        i = 0;
        while( (i = tmp.find("&#36;", i)) != string::npos) {
            tmp.replace(i, 1, "&amp;");
            i += 4;
        }
        i = 0;
        while( (i = tmp.find("&#124;", i)) != string::npos) {
            tmp.replace(i, 1, "&amp;");
            i += 4;
        }
        i = 0;
        while( (i = tmp.find('$', i)) != string::npos) {
            tmp.replace(i, 1, "&#36;");
            i += 4;
        }
        i = 0;
        while( (i = tmp.find('|', i)) != string::npos) {
            tmp.replace(i, 1, "&#124;");
            i += 5;
        }
    }
    return tmp;
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
