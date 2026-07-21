/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "FastAlloc.h"
#include "Flags.h"
#include "GetSet.h"
#include "HintedUser.h"
#include "intrusive_ptr.h"
#include "typedefs.h"
#include "Util.h"

namespace dcpp {

/** Partial-file-sharing source metadata for a queue item. */
class QueuePartialSource : public FastAlloc<QueuePartialSource>, public intrusive_ptr_base<QueuePartialSource> {
public:
    QueuePartialSource(const string& aMyNick, const string& aHubIpPort, const string& aIp, const string& udp) :
        myNick(aMyNick), hubIpPort(aHubIpPort), ip(aIp), udpPort(udp), nextQueryTime(0), pendingQueryCount(0) { }

    ~QueuePartialSource() { }

    typedef dcpp::intrusive_ptr<QueuePartialSource> Ptr;

    GETSET(PartsInfo, partialInfo, PartialInfo);
    GETSET(string, myNick, MyNick);                 // for NMDC support only
    GETSET(string, hubIpPort, HubIpPort);
    GETSET(string, ip, Ip);
    GETSET(string, udpPort, UdpPort);
    GETSET(uint64_t, nextQueryTime, NextQueryTime);
    GETSET(uint8_t, pendingQueryCount, PendingQueryCount);
};

/** One download source for a queue item. */
class QueueSource : public Flags {
public:
    enum {
        FLAG_NONE = 0x00,
        FLAG_FILE_NOT_AVAILABLE = 0x01,
        FLAG_PASSIVE = 0x02,
        FLAG_REMOVED = 0x04,
        FLAG_CRC_FAILED = 0x08,
        FLAG_CRC_WARN = 0x10,
        FLAG_NO_TTHF = 0x20,
        FLAG_BAD_TREE = 0x40,
        FLAG_NO_TREE = 0x80,
        FLAG_SLOW_SOURCE = 0x100,
        FLAG_PARTIAL    = 0x200,
        FLAG_NO_NEED_PARTS      = 0x250,
        FLAG_TTH_INCONSISTENCY  = 0x300,
        FLAG_UNTRUSTED = 0x400,
        FLAG_UNENCRYPTED = 0x450,
        /** Remove-reason only: drop from sources (not stored as a bad source). */
        FLAG_UNREACHABLE = 0x800,
        FLAG_MASK = FLAG_FILE_NOT_AVAILABLE
        | FLAG_PASSIVE | FLAG_REMOVED | FLAG_CRC_FAILED | FLAG_CRC_WARN
        | FLAG_BAD_TREE | FLAG_NO_TREE | FLAG_SLOW_SOURCE | FLAG_TTH_INCONSISTENCY | FLAG_UNTRUSTED
        | FLAG_UNENCRYPTED
    };

    QueueSource(const HintedUser& aUser) : user(aUser), partialSource(NULL) { }
    QueueSource(const QueueSource& aSource) : Flags(aSource), user(aSource.user), partialSource(aSource.partialSource) { }

    bool operator==(const UserPtr& aUser) const { return user == aUser; }
    QueuePartialSource::Ptr& getPartialSource() { return partialSource; }

    GETSET(HintedUser, user, User);
    GETSET(QueuePartialSource::Ptr, partialSource, PartialSource);
};

} // namespace dcpp
