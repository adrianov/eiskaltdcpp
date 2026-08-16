/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

class SearchItem;

/**
 * Distinct holders of one search hit (TTH/dir group): identity merge for Count,
 * currently-online CIDs for Online. Caches until membership or presence changes.
 */
class SearchSources {
public:
    /** Unique sources: same IP once; nick merges with that IP when present, else alone; else CID. */
    int uniqueCount(const SearchItem *group) const;
    /** Distinct CIDs among this file's sources that are online now. */
    int onlineCount(const SearchItem *group) const;
    /** Drop both caches (child added/removed). */
    void invalidate();
    /** Drop only the online cache (user connected or left). */
    void invalidateOnline();

private:
    mutable int uniqueCached = -1;
    mutable int onlineCached = -1;
};
