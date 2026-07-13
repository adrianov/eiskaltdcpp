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
#include "IncomingPortCheck.h"
#include "SearchResult.h"
#include "StringTokenizer.h"
#include "Text.h"
#include "Util.h"
#include "format.h"

namespace dcpp {

int SearchManager::UdpQueue::run() {
    string x = Util::emptyString;
    string remoteIp = Util::emptyString;
    stop = false;
    ;
    while(true) {
        if (resultList.empty())
            s.wait();

        if(stop)
            break;

        {
            Lock l(csudp);
            if(resultList.empty()) continue;

            x = resultList.front().first;
            remoteIp = resultList.front().second;
            resultList.pop_front();
        }

        if(x.compare(0, 4, "$SR ") == 0) {
            string::size_type i, j;
            i = 4;
            if( (j = x.find(' ', i)) == string::npos) {
                continue;
            }
            string nick = x.substr(i, j-i);
            i = j + 1;

            size_t cnt = count(x.begin() + j, x.end(), 0x05);

            SearchResult::Types type = SearchResult::TYPE_FILE;
            string file;
            int64_t size = 0;

            if(cnt == 1) {
                type = SearchResult::TYPE_DIRECTORY;
                if((j = x.rfind(0x05)) == string::npos) {
                    continue;
                }
                if((j = x.rfind(' ', j-1)) == string::npos) {
                    continue;
                }
                if(j < i + 1) {
                    continue;
                }
                file = x.substr(i, j-i) + '\\';
            } else if(cnt == 2) {
                if( (j = x.find((char)5, i)) == string::npos) {
                    continue;
                }
                file = x.substr(i, j-i);
                i = j + 1;
                if( (j = x.find(' ', i)) == string::npos) {
                    continue;
                }
                size = Util::toInt64(x.substr(i, j-i));
            }
            i = j + 1;

            if( (j = x.find('/', i)) == string::npos) {
                continue;
            }
            uint8_t freeSlots = (uint8_t)Util::toInt(x.substr(i, j-i));
            i = j + 1;
            if( (j = x.find((char)5, i)) == string::npos) {
                continue;
            }
            uint8_t slots = (uint8_t)Util::toInt(x.substr(i, j-i));
            i = j + 1;
            // rfind over the whole line breaks when the filename contains " ("
            // (e.g. "Track (+++).avi"); only accept a match after the slots field.
            if( (j = x.rfind(" (")) == string::npos || j < i) {
                continue;
            }
            string hubName = x.substr(i, j-i);
            i = j + 2;
            // Same bound as above: a ')' inside the filename must not win.
            if( (j = x.rfind(')')) == string::npos || j < i) {
                continue;
            }

            string hubIpPort = x.substr(i, j-i);
            string url = ClientManager::getInstance()->findHub(hubIpPort);

            string encoding = ClientManager::getInstance()->findHubEncoding(url);
            nick = Text::toUtf8(nick, encoding);
            file = Text::toUtf8(file, encoding);
            hubName = Text::toUtf8(hubName, encoding);

            UserPtr user = ClientManager::getInstance()->findUser(nick, url);
            if(!user) {
                user = ClientManager::getInstance()->findLegacyUser(nick);
                if(!user)
                    continue;
            }

            ClientManager::getInstance()->setIPUser(user, remoteIp);

            string tth;
            if(hubName.compare(0, 4, "TTH:") == 0) {
                tth = hubName.substr(4);
                StringList names = ClientManager::getInstance()->getHubNames(user->getCID(), Util::emptyString);
                hubName = names.empty() ? _("Offline") : Util::toString(names);
            }

            if(tth.empty() && type == SearchResult::TYPE_FILE) {
                continue;
            }

            SearchResultPtr sr(new SearchResult(user, type, slots, freeSlots, size,
                                                file, hubName, url, remoteIp, TTHValue(tth), Util::emptyString));
            SearchManager::getInstance()->fire(SearchManagerListener::SR(), sr);

        } else if(x.compare(1, 4, "RES ") == 0 && x[x.length() - 1] == 0x0a) {
            AdcCommand c(x.substr(0, x.length()-1));
            if(c.getParameters().empty())
                continue;
            string cid = c.getParam(0);
            if(cid.size() != 39)
                continue;

            UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
            if(!user)
                continue;

            c.getParameters().erase(c.getParameters().begin());

            SearchManager::getInstance()->onRES(c, user, remoteIp);

        } else if(x.compare(1, 4, "PSR ") == 0 && x[x.length() - 1] == 0x0a) {
            AdcCommand c(x.substr(0, x.length()-1));
            if(c.getParameters().empty())
                continue;
            string cid = c.getParam(0);
            if(cid.size() != 39)
                continue;

            UserPtr user = ClientManager::getInstance()->findUser(CID(cid));

            c.getParameters().erase(c.getParameters().begin());

            SearchManager::getInstance()->onPSR(c, user, remoteIp);

        }

        Thread::sleep(10);
    }
    return 0;
}

void SearchManager::onData(const uint8_t* buf, size_t aLen, const string& remoteIp) {
    if(!remoteIp.empty() && !port.empty())
        IncomingPortCheck::getInstance()->noteOpen("UDP", port);

    string x((char*)buf, aLen);
    queue.addResult(x, remoteIp);
}

} // namespace dcpp
