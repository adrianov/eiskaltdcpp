/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"

#include "File.h"
#include "SettingsManager.h"
#include "TimerManager.h"
#include "Util.h"

namespace dcpp {

namespace {

constexpr uint64_t kTrimInterval = 60 * 1000;

FastCriticalSection trimCs;
unordered_map<string, uint64_t> lastTrim;

void copyRest(File& in, File& out) {
    char buf[8192];
    for (;;) {
        size_t len = sizeof(buf);
        if(in.read(buf, len) == 0)
            break;
        out.write(buf, len);
    }
}

bool claimTrim(const string& path) {
    FastLock l(trimCs);
    const uint64_t tick = GET_TICK();
    auto it = lastTrim.find(path);
    if(it != lastTrim.end() && tick < it->second + kTrimInterval)
        return false;
    lastTrim[path] = tick;
    return true;
}

void markTrimmed(const string& path) {
    FastLock l(trimCs);
    lastTrim[path] = GET_TICK();
}

void clearTrimClaim(const string& path) {
    FastLock l(trimCs);
    lastTrim.erase(path);
}

} // namespace

void trimLogFile(const string& path) {
    const int maxMb = SettingsManager::getInstance()->get(SettingsManager::LOG_MAX_FILE_SIZE, true);
    if(maxMb <= 0 || !Util::fileExists(path))
        return;

    try {
        const int64_t maxBytes = static_cast<int64_t>(maxMb) * 1024 * 1024;
        const int64_t size = File::getSize(path);
        if(size <= 0 || size <= maxBytes || !claimTrim(path))
            return;

        const int64_t keepBytes = maxBytes * 9 / 10;
        int64_t skipPos = size - keepBytes;
        if(skipPos < 0)
            skipPos = 0;

        File in(path, File::READ, File::OPEN);
        if(skipPos > 0) {
            in.setPos(skipPos);
            char buf[4096];
            size_t len = sizeof(buf);
            in.read(buf, len);
            const string chunk(buf, len);
            const auto nl = chunk.find('\n');
            if(nl != string::npos)
                in.setPos(skipPos + static_cast<int64_t>(nl) + 1);
        }

        const string tmp = path + ".trimtmp";
        File out(tmp, File::WRITE, File::OPEN | File::CREATE | File::TRUNCATE);
        copyRest(in, out);
        in.close();
        out.close();
        File::renameFile(tmp, path);
        markTrimmed(path);
    } catch (const FileException&) {
        clearTrimClaim(path);
    }
}

} // namespace dcpp
