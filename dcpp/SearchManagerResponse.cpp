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

#include "SearchManager.h"

#include "AdcCommand.h"
#include "ClientManager.h"
#include "FinishedManager.h"
#include "QueueItem.h"
#include "QueueManager.h"
#include "SearchResult.h"
#include "ShareManager.h"
#include "StringTokenizer.h"
#include "Text.h"
#include "Util.h"
#include "format.h"

namespace dcpp {

void SearchManager::onRES(const AdcCommand& cmd, const UserPtr& from, const string& remoteIp) {
    int freeSlots = -1;
    int64_t size = -1;
    string file;
    string tth;
    string token;

    for(StringIterC i = cmd.getParameters().begin(); i != cmd.getParameters().end(); ++i) {
        const string& str = *i;
        if(str.compare(0, 2, "FN") == 0) {
            file = Util::toNmdcFile(str.substr(2));
        } else if(str.compare(0, 2, "SL") == 0) {
            freeSlots = Util::toInt(str.substr(2));
        } else if(str.compare(0, 2, "SI") == 0) {
            size = Util::toInt64(str.substr(2));
        } else if(str.compare(0, 2, "TR") == 0) {
            tth = str.substr(2);
        } else if(str.compare(0, 2, "TO") == 0) {
            token = str.substr(2);
        }
    }

    if(!file.empty() && freeSlots != -1 && size != -1) {

        StringList names = ClientManager::getInstance()->getHubNames(from->getCID(), Util::emptyString);
        string hubName = names.empty() ? _("Offline") : Util::toString(names);
        StringList hubs = ClientManager::getInstance()->getHubs(from->getCID(), Util::emptyString);
        string hub = hubs.empty() ? _("Offline") : Util::toString(hubs);

        SearchResult::Types type = (file[file.length() - 1] == '\\' ? SearchResult::TYPE_DIRECTORY : SearchResult::TYPE_FILE);
        if(type == SearchResult::TYPE_FILE && tth.empty())
            return;

        uint8_t slots = ClientManager::getInstance()->getSlots(from->getCID());
        SearchResultPtr sr(new SearchResult(from, type, slots, (uint8_t)freeSlots, size,
                                            file, hubName, hub, remoteIp, TTHValue(tth), token));
        fire(SearchManagerListener::SR(), sr);
    }
}

void SearchManager::onPSR(const AdcCommand& cmd, UserPtr from, const string& remoteIp) {

    string udpPort;
    uint32_t partialCount = 0;
    string tth;
    string hubIpPort;
    string nick;
    PartsInfo partialInfo;

    for(StringIterC i = cmd.getParameters().begin(); i != cmd.getParameters().end(); ++i) {
        const string& str = *i;
        if(str.compare(0, 2, "U4") == 0) {
            udpPort = str.substr(2);
        } else if(str.compare(0, 2, "NI") == 0) {
            nick = str.substr(2);
        } else if(str.compare(0, 2, "HI") == 0) {
            hubIpPort = str.substr(2);
        } else if(str.compare(0, 2, "TR") == 0) {
            tth = str.substr(2);
        } else if(str.compare(0, 2, "PC") == 0) {
            partialCount = Util::toUInt32(str.substr(2))*2;
        } else if(str.compare(0, 2, "PI") == 0) {
            StringTokenizer<string> tok(str.substr(2), ',');
            for(auto& i : tok.getTokens()) {
                partialInfo.push_back((uint16_t)Util::toInt(i));
            }
        }
    }

    string url = ClientManager::getInstance()->findHub(hubIpPort);
    if(!from || from == ClientManager::getInstance()->getMe()) {

        if(nick.empty() || hubIpPort.empty()) {
            return;
        }

        from = ClientManager::getInstance()->findUser(nick, url);
        if(!from) {
            from = ClientManager::getInstance()->findLegacyUser(nick);
            if(!from) {
                dcdebug("Search result from unknown user");
                return;
            }
        }
    }

    ClientManager::getInstance()->setIPUser(from, remoteIp, udpPort);

    if(partialInfo.size() != partialCount) {
        return;
    }

    PartsInfo outPartialInfo;
    QueueItem::PartialSource ps(from->isNMDC() ? ClientManager::getInstance()->getClient(url)->getMyIdentity().getNick() : Util::emptyString, hubIpPort, remoteIp, udpPort);
    ps.setPartialInfo(partialInfo);

    QueueManager::getInstance()->handlePartialResult(from, url, TTHValue(tth), ps, outPartialInfo);

    if((Util::toInt(udpPort) > 0) && !outPartialInfo.empty()) {
        try {
            AdcCommand cmd = SearchManager::getInstance()->toPSR(false, ps.getMyNick(), hubIpPort, tth, outPartialInfo);
            ClientManager::getInstance()->send(cmd, from->getCID());
        } catch(...) {
            dcdebug("Partial search caught error\n");
        }
    }

}

void SearchManager::respond(const AdcCommand& adc, const CID& from,  bool isUdpActive, const string& hubIpPort) {
    if(from == ClientManager::getInstance()->getMe()->getCID())
        return;

    UserPtr p = ClientManager::getInstance()->findUser(from);
    if(!p)
        return;

    SearchResultList results;
    ShareManager::getInstance()->search(results, adc.getParameters(), isUdpActive ? 10 : 5);

    string token;

    adc.getParam("TO", 0, token);

    if(results.empty()) {
        string tth;
        if(!adc.getParam("TR", 0, tth))
            return;

        PartsInfo partialInfo;
        if(!QueueManager::getInstance()->handlePartialSearch(TTHValue(tth), partialInfo)) {
            if(!FinishedManager::getInstance()->handlePartialRequest(TTHValue(tth), partialInfo)) {
                return;
            }
        }

        AdcCommand cmd = toPSR(true, Util::emptyString, hubIpPort, tth, partialInfo);
        ClientManager::getInstance()->send(cmd, from);
        return;
    }

    for(SearchResultList::const_iterator i = results.begin(); i != results.end(); ++i) {
        AdcCommand cmd = (*i)->toRES(AdcCommand::TYPE_UDP);
        if(!token.empty())
            cmd.addParam("TO", token);
        ClientManager::getInstance()->send(cmd, from);
    }
}

string SearchManager::getPartsString(const PartsInfo& partsInfo) const {
    string ret;

    for(PartsInfo::const_iterator i = partsInfo.begin(); i < partsInfo.end(); i+=2){
        ret += Util::toString(*i) + "," + Util::toString(*(i+1)) + ",";
    }

    return ret.substr(0, ret.size()-1);
}

AdcCommand SearchManager::toPSR(bool wantResponse, const string& myNick, const string& hubIpPort, const string& tth, const vector<uint16_t>& partialInfo) const {
    AdcCommand cmd(AdcCommand::CMD_PSR, AdcCommand::TYPE_UDP);

    if(!myNick.empty())
        cmd.addParam("NI", Text::utf8ToAcp(myNick));

    cmd.addParam("HI", hubIpPort);
    cmd.addParam("U4", (wantResponse && ClientManager::getInstance()->isActive(hubIpPort)) ? SearchManager::getInstance()->getPort() : Util::emptyString);
    cmd.addParam("TR", tth);
    cmd.addParam("PC", Util::toString(partialInfo.size() / 2));
    cmd.addParam("PI", getPartsString(partialInfo));

    return cmd;
}

} // namespace dcpp
