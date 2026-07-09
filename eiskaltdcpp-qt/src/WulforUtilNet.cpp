/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "WulforUtil.h"

#ifdef HAVE_IFADDRS_H
#include <sys/types.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <fcntl.h>
#endif

#include <QNetworkInterface>
#include <QHostAddress>
#include <QStringList>

QStringList WulforUtil::getLocalIfaces(){
    QStringList ifaces;

#ifdef Q_OS_HAIKU
#undef HAVE_IFADDRS_H
#endif // Q_OS_HAIKU

#ifdef HAVE_IFADDRS_H
    struct ifaddrs *ifap;

    if (getifaddrs(&ifap) == 0){
        for (struct ifaddrs *i = ifap; i ; i = i->ifa_next){
            struct sockaddr *sa = i->ifa_addr;

            // If the interface is up, is not a loopback and it has an address
            if ((i->ifa_flags & IFF_UP) && !(i->ifa_flags & IFF_LOOPBACK) && sa && !ifaces.contains(i->ifa_name))
                ifaces.push_back(i->ifa_name);
        }

        freeifaddrs(ifap);
    }
#endif

    return ifaces;
}

QStringList WulforUtil::getLocalIPs(){
    QStringList addresses;

#ifdef HAVE_IFADDRS_H
    struct ifaddrs *ifap;

    if (getifaddrs(&ifap) == 0){
        for (struct ifaddrs *i = ifap; i ; i = i->ifa_next){
            struct sockaddr *sa = i->ifa_addr;

            // If the interface is up, is not a loopback and it has an address
            if ((i->ifa_flags & IFF_UP) && !(i->ifa_flags & IFF_LOOPBACK) && sa){
                void* src = nullptr;
                socklen_t len;

                // IPv4 address
                if (sa->sa_family == AF_INET){
                    struct sockaddr_in* sai = (struct sockaddr_in*)sa;
                    src = (void*) &(sai->sin_addr);
                    len = INET_ADDRSTRLEN;
                }
                // IPv6 address
                else if (sa->sa_family == AF_INET6){
                    struct sockaddr_in6* sai6 = (struct sockaddr_in6*)sa;
                        src = (void*) &(sai6->sin6_addr);
                        len = INET6_ADDRSTRLEN;
                }

                // Convert the binary address to a string and add it to the output list
                if (src){
                    char address[len];
                    inet_ntop(sa->sa_family, src, address, len);
                    addresses.push_back(address);
                }
            }
        }

        freeifaddrs(ifap);
    }
#else
    addresses.push_back(Util::getLocalIp().c_str());
#endif

    return addresses;
}

