/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "MediaInfo.h"
#include "MerkleTree.h"
#include "Singleton.h"
#include "CriticalSection.h"

#include <unordered_map>

namespace dcpp {

/** Persist scanned media by TTH (Flylink fly_file media columns). */
class MediaInfoCache : public Singleton<MediaInfoCache>
{
public:
    friend class Singleton<MediaInfoCache>;

    bool get(const TTHValue& tth, MediaInfo& out);
    void put(const TTHValue& tth, const MediaInfo& info);
    void save();

private:
    MediaInfoCache() = default;
    ~MediaInfoCache() { save(); }

    void load();
    static string cachePath();

    CriticalSection cs;
    std::unordered_map<TTHValue, MediaInfo> map;
    bool loaded = false;
    bool dirty = false;
};

} // namespace dcpp
