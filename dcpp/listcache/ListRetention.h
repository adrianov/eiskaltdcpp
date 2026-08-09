/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <ctime>

#include "../typedefs.h"

namespace dcpp {

/**
 * File-list retention: import legacy sidecars and expire FileLists/ past maxAge
 * (default 1 day, same window as fetch cooldown).
 */
class ListRetention {
public:
    static const time_t DEFAULT_MAX_AGE = 24 * 60 * 60;

    explicit ListRetention(time_t maxAge = DEFAULT_MAX_AGE) : maxAge(maxAge) {}

    /** Migrate .sharesize/.listfetch sidecars, then expire stale lists. */
    void onStartup();
    /** Remove FileLists older than maxAge and matching ListCache.xml rows. */
    void expire();

private:
    time_t maxAge;
    time_t cutoff = 0;

    StringList migrateSidecars();
    bool expirePath(const string& path);
    bool pruneMeta();
    static string cidFromPath(const string& path);
    static time_t fileMtime(const string& path);
    time_t ageOf(const string& path, const string& cid) const;
};

} // namespace dcpp
