/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "../SearchManager.h"
#include "../typedefs.h"

namespace dcpp {

/**
 * Classifies shared filenames into SearchManager type buckets by extension.
 * Used when tagging the share tree and when answering hub searches.
 */
class ShareFileType {
public:
    /** True when fileName belongs to the given SearchManager::TYPE_* bucket. */
    static bool matches(const string& fileName, int type);
    /** First matching type, or TYPE_DIRECTORY / TYPE_ANY. */
    static SearchManager::TypeModes classify(const string& fileName) noexcept;

private:
    struct Bucket {
        int type;
        const char* const* shortExts;
        size_t shortCount;
        const string* longExts;
        size_t longCount;
    };

    static const Bucket* buckets(size_t& count);
    static uint32_t packExt3(const string& fileName);
    static bool matchShort(uint32_t packed, const char* const* exts, size_t count);
    static bool matchLong(const string& fileName, const string* exts, size_t count);
    static bool matchBucket(const string& fileName, uint32_t packed, const Bucket& bucket);
};

} // namespace dcpp
