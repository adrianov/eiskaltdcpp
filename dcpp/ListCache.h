/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <cstdint>
#include <ctime>
#include <string>

#include "CID.h"
#include "HintedUser.h"

namespace dcpp {

using std::string;

/** Per-user file list cache metadata in PATH_USER_LOCAL/ListCache.xml. */
class ListCache {
public:
    static void load();

    static string findListFile(const string& listBase);
    /** True when the online user's reported share size matches cached metadata. */
    static bool matchesShare(const UserPtr& user);
    static bool matchesUserShare(const HintedUser& user, const string& listBase);
    /**
     * Empty FileListing.xml.bz2 is ~200 bytes. Reject that as a cache hit when the hub
     * advertises a real share — usually a wrong-peer or stub list.
     */
    static bool isPlausibleList(int64_t shareSize, int64_t listSize);
    /** Downloaded XML/XML.BZ2 byte size, or -1 when no size was recorded. */
    static int64_t fileSize(const CID& cid);
    static void saveListMeta(const CID& cid, int64_t shareSize, int64_t fileSize,
                             time_t when = time(nullptr));
    /** True when a successful fetch was recorded less than 24 hours ago (auto-refresh cooldown). */
    static bool fetchedWithinDay(const CID& cid);
};

} // namespace dcpp
