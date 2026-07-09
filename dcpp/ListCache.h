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

#include "HintedUser.h"

namespace dcpp {

using std::string;

/** Saved user file list reuse when hub share size (SS) is unchanged. */
class ListCache {
public:
    static string findListFile(const string& listBase);
    static int64_t readShareSize(const string& listBase);
    static void saveShareSize(const string& listBase, int64_t shareSize);
    static bool matchesUserShare(const HintedUser& user, const string& listBase);

    /** Unix time of last successful full-list download; -1 if unknown. */
    static time_t readFetchTime(const string& listBase);
    static void saveFetchTime(const string& listBase, time_t when = time(nullptr));
    /** True when a fetch was recorded less than 24 hours ago. */
    static bool fetchedWithinDay(const string& listBase);
};

} // namespace dcpp
