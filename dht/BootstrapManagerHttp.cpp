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
#include "Utils.h"
#include "dcpp/AdcCommand.h"
#include "dcpp/LogManager.h"
#include <zlib.h>

namespace dht
{

    namespace {
        bool isIpv4(const string& ip)
        {
            return !ip.empty() && ip.find(':') == string::npos && inet_addr(ip.c_str()) != INADDR_NONE;
        }

        size_t addSeedNodes(BootstrapManager& bm, const string& xml)
        {
            uLongf destLen = 16384;
            std::unique_ptr<uint8_t[]> destBuf;
            int result;
            do
            {
                destLen *= 2;
                destBuf.reset(new uint8_t[destLen]);
                result = uncompress(&destBuf[0], &destLen, (Bytef*)xml.data(), xml.length());
            }
            while (result == Z_BUF_ERROR);

            if(result != Z_OK)
                throw Exception("Decompress error.");

            SimpleXML remoteXml;
            remoteXml.fromXML(string((char*)&destBuf[0], destLen));
            remoteXml.stepIn();

            size_t added = 0;
            while(remoteXml.findChild("Node"))
            {
                CID cid = CID(remoteXml.getChildAttrib("CID"));
                string i4 = remoteXml.getChildAttrib("I4");
                string u4 = remoteXml.getChildAttrib("U4");
                // DHT UDP is IPv4-only; skip IPv6 seeds from the HTTP list.
                if(!isIpv4(i4) || !Utils::isGoodIPPort(i4, u4))
                    continue;
                bm.addBootstrapNode(i4, u4, cid, UDPKey());
                ++added;
            }
            remoteXml.stepOut();
            return added;
        }
    }

    void BootstrapManager::on(HttpConnectionListener::Data, HttpConnection*, const uint8_t* buf, size_t len) throw()
    {
        Lock l(cs);
        nodesXML += string((const char*)buf, len);
    }

    void BootstrapManager::on(HttpConnectionListener::Complete, HttpConnection*, string const&) throw()
    {
        string xml;
        {
            Lock l(cs);
            finishDownload();
            xml.swap(nodesXML);
        }

        if(xml.empty())
        {
            LogManager::getInstance()->message(_("DHT bootstrap error: empty response"));
            return;
        }

        try
        {
            size_t added = addSeedNodes(*this, xml);
            if(added > 0)
                LogManager::getInstance()->message(_("DHT bootstrapping is finished successfully."));
            else
                LogManager::getInstance()->message(_("DHT bootstrap error: no usable IPv4 nodes in seed list"));
        }
        catch(Exception& e)
        {
            LogManager::getInstance()->message(_("DHT bootstrap error: ") + e.getError());
        }
    }

    void BootstrapManager::on(HttpConnectionListener::Failed, HttpConnection*, const string& aLine) throw()
    {
        {
            Lock l(cs);
            finishDownload();
            nodesXML.clear();
        }
        LogManager::getInstance()->message(_("DHT bootstrap error: ") + aLine);
    }

    void BootstrapManager::process()
    {
        Lock l(cs);
        if(bootstrapNodes.empty())
            return;

        AdcCommand cmd(AdcCommand::CMD_GET, AdcCommand::TYPE_UDP);
        cmd.addParam("nodes");
        cmd.addParam("dht.xml");

        const BootstrapNode& node = bootstrapNodes.front();

        CID key;
        if(DHT::getInstance()->getLastExternalIP() == node.udpKey.ip)
            key = node.udpKey.key;

        DHT::getInstance()->send(cmd, node.ip, node.udpPort, node.cid, key);
        bootstrapNodes.pop_front();
    }

}
