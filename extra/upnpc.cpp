/*
 * Copyright (C) 2001-2011 Jacek Sieka, arnetheduck on gmail point com
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
 *
 * In addition, as a special exception, compiling, linking, and/or
 * using OpenSSL with this program is allowed.
 *
 * This program uses the MiniUPnP client library by Thomas Bernard
 * http://miniupnp.free.fr https://miniupnp.tuxfamily.org/
 */

#include "upnpc.h"
#include "dcpp/Util.h"
#include "dcpp/SettingsManager.h"
#ifndef STATICLIB
#define STATICLIB
#endif
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>

#include <algorithm>

static UPNPUrls urls;
static IGDdatas data;
static char lanaddr[64];
static bool haveUrls = false;
const std::string UPnPc::name = "MiniUPnP";

using namespace std;
using namespace dcpp;

static UPNPDev* discover(const char* iface)
{
#if (MINIUPNPC_API_VERSION >= 14)
    return upnpDiscover(5000, iface, nullptr, 0, 0, 2, nullptr);
#else
    return upnpDiscover(5000, iface, nullptr, 0, 0, nullptr);
#endif
}

bool UPnPc::init()
{
    if (haveUrls) {
        FreeUPNPUrls(&urls);
        haveUrls = false;
    }

    // Use the configured bind address only while it is assigned to an interface;
    // a stale address would make SSDP discovery fail every time.
    const string bind_address = SETTING(BIND_ADDRESS);
    const char* iface = nullptr;
    if (!SettingsManager::getInstance()->isDefault(SettingsManager::BIND_ADDRESS)) {
        const auto ips = Util::getLocalIPs(AF_INET);
        if (std::find(ips.begin(), ips.end(), bind_address) != ips.end())
            iface = bind_address.c_str();
    }

    UPNPDev *devices = discover(iface);
    if (!devices && iface)
        devices = discover(nullptr);
    if (!devices)
        return false;

    lanaddr[0] = 0;
#if (MINIUPNPC_API_VERSION >= 18)
    int ret = UPNP_GetValidIGD(devices, &urls, &data, lanaddr, sizeof(lanaddr), nullptr, 0);
    const int worstUsable = 3; // 2 = private-IP IGD, 3 = disconnected IGD
#else
    int ret = UPNP_GetValidIGD(devices, &urls, &data, lanaddr, sizeof(lanaddr));
    const int worstUsable = 2; // 2 = IGD not connected; 3 = not an IGD
#endif

    freeUPNPDevlist(devices);

    haveUrls = (ret != 0);
    return ret >= 1 && ret <= worstUsable;
}

bool UPnPc::add(const string& port, const UPnP::Protocol protocol, const string& description)
{
    // The IGD reports which of our addresses routes to it; trust that over guessing.
    const string local = lanaddr[0] ? lanaddr : Util::getLocalIp(AF_INET);

    if (UPNP_AddPortMapping(urls.controlURL, data.first.servicetype, port.c_str(), port.c_str(),
            local.c_str(), description.c_str(), protocols[protocol], nullptr, nullptr) == UPNPCOMMAND_SUCCESS)
        return true;

    // A stale mapping left by an unclean shutdown blocks the port; drop it and retry.
    UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, port.c_str(), protocols[protocol], nullptr);
    return UPNP_AddPortMapping(urls.controlURL, data.first.servicetype, port.c_str(), port.c_str(),
        local.c_str(), description.c_str(), protocols[protocol], nullptr, nullptr) == UPNPCOMMAND_SUCCESS;
}

bool UPnPc::remove(const string& port, const UPnP::Protocol protocol)
{
    return UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, port.c_str(),
        protocols[protocol], nullptr) == UPNPCOMMAND_SUCCESS;
}

string UPnPc::getExternalIP()
{
    char buf[16] = { 0 };
    if (UPNP_GetExternalIPAddress(urls.controlURL, data.first.servicetype, buf) == UPNPCOMMAND_SUCCESS)
        return string(buf);
    return Util::emptyString;
}
