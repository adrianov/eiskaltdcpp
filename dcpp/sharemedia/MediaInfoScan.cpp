/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 * Adapted from FlylinkDC++ getMediaInfo (flyServer.cpp).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "MediaInfoScan.h"
#include "MediaInfoCache.h"
#include "MediaInfoProbe.h"
#include "SettingsManager.h"
#include "Text.h"
#include "Util.h"
#include "File.h"
#include "LogManager.h"

namespace dcpp {

namespace {

// Flylink compiled/Settings mediainfo_ext (+ mxf).
const char* mediaExts[] = {
    "3gp", "avi", "divx", "flv", "m4v", "mkv", "dts", "mov", "mp4", "mpg", "mpeg",
    "vob", "wmv", "bik", "qt", "rm", "aac", "ac3", "ape", "fla", "flac", "m4a",
    "mp1", "mp2", "mp3", "ogg", "wma", "wv", "mka", "vqf", "lqt", "ts", "tp",
    "mod", "m2ts", "webm", "mpe", "mxf", nullptr
};

string fileExt(const string& path)
{
    string e = Util::getFileExt(path);
    if (!e.empty() && e[0] == '.')
        e.erase(0, 1);
    return Text::toLower(e);
}

} // namespace

bool isMediaInfoExt(const string& ext)
{
    const string e = Text::toLower(ext);
    for (const char** p = mediaExts; *p; ++p) {
        if (e == *p)
            return true;
    }
    return false;
}

bool mediaInfoFill(const string& path, int64_t size, const TTHValue& tth, MediaInfo& out)
{
    out = MediaInfo();
#ifndef USE_MEDIAINFO
    (void)path;
    (void)size;
    (void)tth;
    return false;
#else
    if (MediaInfoCache::getInstance()->get(tth, out))
        return !out.empty();

    if (size < 0)
        size = File::getSize(path);
    if (size < int64_t(SETTING(MIN_MEDIAINFO_SIZE)) * 1024 * 1024)
        return false;
    if (!isMediaInfoExt(fileExt(path)))
        return false;

    try {
        const bool ok = MediaInfoProbe::scan(path, out);
        MediaInfoCache::getInstance()->put(tth, out);
        return ok;
    } catch (const std::exception& e) {
        LogManager::getInstance()->message(string("MediaInfo: ") + path + " (" + e.what() + ")");
        MediaInfoCache::getInstance()->put(tth, out);
        return false;
    } catch (...) {
        MediaInfoCache::getInstance()->put(tth, out);
        return false;
    }
#endif
}

} // namespace dcpp
