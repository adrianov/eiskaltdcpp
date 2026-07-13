/*
 * Copyright (C) 2009-2010 Big Muscle, http://strongdc.sourceforge.net/
 * Copyright (C) 2019 Boris Pek <tehnick-8@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "stdafx.h"
#include "BootstrapManager.h"
#include "DHT.h"
#include "dcpp/ClientManager.h"
#include "dcpp/HttpConnection.h"
#include "dcpp/LogManager.h"

namespace dht
{

    namespace {
        // Community-hosted seed list (legacy strongdc/fly-server endpoints are gone).
        const char* const BOOTSTRAP_URLS[] = {
            "https://dht.hublist.eu/dcDHT.php"
        };
        const size_t BOOTSTRAP_URL_COUNT = sizeof(BOOTSTRAP_URLS) / sizeof(BOOTSTRAP_URLS[0]);
        const uint64_t HTTP_RETRY_MS = 60 * 1000;
    }

    BootstrapManager::BootstrapManager(void) :
        nextServer(0), downloading(false), callActive(false), syncFinished(false), lastAttempt(0)
    {
        httpConnection.addListener(this);
    }

    BootstrapManager::~BootstrapManager(void)
    {
        httpConnection.removeListener(this);
    }

    void BootstrapManager::finishDownload()
    {
        syncFinished = true;
        if(!callActive)
            downloading = false;
    }

    void BootstrapManager::bootstrap()
    {
        string url;
        {
            Lock l(cs);
            if(!bootstrapNodes.empty() || downloading)
                return;
            if(lastAttempt && GET_TICK() - lastAttempt < HTTP_RETRY_MS)
                return;

            lastAttempt = GET_TICK();
            downloading = true;
            callActive = true;
            syncFinished = false;
            nodesXML.clear();

            const string& dhturl = BOOTSTRAP_URLS[nextServer++ % BOOTSTRAP_URL_COUNT];
            url = string(dhturl) + "?cid=" + ClientManager::getInstance()->getMe()->getCID().toBase32()
                + "&encryption=1&u4=" + DHT::getInstance()->getPort();
        }

        // Outside the lock: Failed/Complete may run before downloadFile returns.
        // Keep downloading set until we leave downloadFile so another thread cannot
        // start a second attempt on the same HttpConnection mid-call.
        LogManager::getInstance()->message(_("DHT bootstrapping started"));
        httpConnection.downloadFile(url);

        Lock l(cs);
        callActive = false;
        if(syncFinished)
            downloading = false;
    }

    void BootstrapManager::addBootstrapNode(const string& ip, const string& udpPort, const CID& targetCID, const UDPKey& udpKey)
    {
        Lock l(cs);
        BootstrapNode node = { ip, udpPort, targetCID, udpKey };
        bootstrapNodes.push_back(node);
    }

}
