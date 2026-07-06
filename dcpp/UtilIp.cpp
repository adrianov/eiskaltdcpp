/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/** IP address helpers: local interface discovery, IPv4 validation, ip:port parsing. */

#include "stdinc.h"

#include "Util.h"

#ifdef _WIN32
#include "w.h"
#include <iphlpapi.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#ifdef HAVE_IFADDRS_H
#include <cstring>
#include <ifaddrs.h>
#include <net/if.h>
#endif

#ifdef __HAIKU__
#undef HAVE_IFADDRS_H
#endif

namespace dcpp {

void Util::parseIpPort(const string& aIpPort, string& ip, string& port) {
    string::size_type i = aIpPort.rfind(':');
    if (i == string::npos) {
        ip = aIpPort;
    } else {
        ip = aIpPort.substr(0, i);
        port = aIpPort.substr(i + 1);
    }
}

vector<string> Util::getLocalIPs(unsigned short sa_family) {
    vector<string> addresses;

#ifdef HAVE_IFADDRS_H
    struct ifaddrs *ifap;

    if (getifaddrs(&ifap) == 0)
    {
        bool ipv4 = (sa_family == AF_UNSPEC) || (sa_family == AF_INET);
        bool ipv6 = (sa_family == AF_UNSPEC) || (sa_family == AF_INET6);

        for (struct ifaddrs *i = ifap; i != NULL; i = i->ifa_next) {
            struct sockaddr *sa = i->ifa_addr;

            // If the interface is up, is not a loopback and it has an address
            if ((i->ifa_flags & IFF_UP) && !(i->ifa_flags & IFF_LOOPBACK) && sa != NULL) {
                void* src = nullptr;
                socklen_t len;

                if (ipv4 && (sa->sa_family == AF_INET)) {
                    // IPv4 address
                    struct sockaddr_in* sai = (struct sockaddr_in*)sa;
                    src = (void*) &(sai->sin_addr);
                    len = INET_ADDRSTRLEN;
                } else if (ipv6 && (sa->sa_family == AF_INET6)) {
                    // IPv6 address
                    struct sockaddr_in6* sai6 = (struct sockaddr_in6*)sa;
                    src = (void*) &(sai6->sin6_addr);
                    len = INET6_ADDRSTRLEN;
                }

                // Convert the binary address to a string and add it to the output list
                if (src) {
                    char address[len];
                    inet_ntop(sa->sa_family, src, address, len);
                    addresses.push_back(address);
                }
            }
        }
        freeifaddrs(ifap);
    }
#endif

    return addresses;
}

string Util::getLocalIp(unsigned short as_family) {
#ifdef HAVE_IFADDRS_H
    vector<string> addresses = getLocalIPs(as_family);
    if (addresses.empty())
        return (((as_family == AF_UNSPEC) || (as_family == AF_INET)) ? "0.0.0.0" : "::");

    const bool wantV4 = (as_family == AF_UNSPEC) || (as_family == AF_INET);
    const bool wantV6 = (as_family == AF_UNSPEC) || (as_family == AF_INET6);
    string best;

    if(wantV4) {
        for(const auto& ip : addresses) {
            if(ip.find(':') != string::npos)
                continue;
            if(Util::isPrivateIp(ip) && ::strncmp(ip.c_str(), "169", 3) != 0) {
                best = ip;
                break;
            }
        }
        if(best.empty()) {
            for(const auto& ip : addresses) {
                if(ip.find(':') == string::npos && ::strncmp(ip.c_str(), "169", 3) != 0) {
                    best = ip;
                    break;
                }
            }
        }
    }
    if(best.empty() && wantV6) {
        for(const auto& ip : addresses) {
            if(ip.find(':') != string::npos && ip.compare(0, 4, "fe80") != 0) {
                best = ip;
                break;
            }
        }
    }
    if(!best.empty())
        return best;
    if(wantV4) {
        for(const auto& ip : addresses) {
            if(ip.find(':') == string::npos)
                return ip;
        }
        return "0.0.0.0";
    }
    return addresses[0];
#else
    string tmp;

    char buf[256];
    gethostname(buf, 255);
    hostent* he = gethostbyname(buf);
    if(he == NULL || he->h_addr_list[0] == 0)
        return Util::emptyString;
    sockaddr_in dest;
    int i = 0;

    // We take the first ip as default, but if we can find a better one, use it instead...
    memcpy(&(dest.sin_addr), he->h_addr_list[i++], he->h_length);
    tmp = inet_ntoa(dest.sin_addr);
    if(Util::isPrivateIp(tmp) || ::strncmp(tmp.c_str(), "169", 3) == 0) {
        while(he->h_addr_list[i]) {
            memcpy(&(dest.sin_addr), he->h_addr_list[i], he->h_length);
            string tmp2 = inet_ntoa(dest.sin_addr);
            if(!Util::isPrivateIp(tmp2) && ::strncmp(tmp2.c_str(), "169", 3) != 0) {
                tmp = tmp2;
            }
            i++;
        }
    }
    return tmp;
#endif
}

string Util::normalizeIpv4(const string& ip) {
    const string::size_type b = ip.find_first_not_of(" \t\r\n");
    if(b == string::npos)
        return Util::emptyString;
    const string::size_type e = ip.find_last_not_of(" \t\r\n");
    const string s = ip.substr(b, e - b + 1);

    in_addr addr;
    addr.s_addr = inet_addr(s.c_str());
    // Round-trip comparison rejects IPv6, partial forms like "1.2.3", 0.0.0.0 and broadcast.
    if(addr.s_addr == INADDR_NONE || addr.s_addr == INADDR_ANY || s != inet_ntoa(addr))
        return Util::emptyString;
    return s;
}

bool Util::isPrivateIp(string const& ip) {
    struct in_addr addr;

    addr.s_addr = inet_addr(ip.c_str());

    if (addr.s_addr != INADDR_NONE) {
        unsigned long haddr = ntohl(addr.s_addr);
        return ((haddr & 0xff000000) == 0x0a000000 || // 10.0.0.0/8
                (haddr & 0xff000000) == 0x7f000000 || // 127.0.0.0/8
                (haddr & 0xfff00000) == 0xac100000 || // 172.16.0.0/12
                (haddr & 0xffff0000) == 0xc0a80000);  // 192.168.0.0/16
    }
    return false;
}

} // namespace dcpp
