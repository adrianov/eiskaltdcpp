/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"

#include "File.h"
#include "SettingsManager.h"
#include "Util.h"

namespace dcpp {

namespace {

constexpr int kKeepLines = 10000;
constexpr int kMaxDepth = 6;

void copyRest(File& in, File& out) {
    char buf[8192];
    for (;;) {
        size_t len = sizeof(buf);
        if(in.read(buf, len) == 0)
            break;
        out.write(buf, len);
    }
}

/** Byte offset of the first kept line, or 0 if the file has at most kKeepLines lines. */
int64_t keepFrom(File& in, int64_t size) {
    if(size <= 0)
        return 0;

    int need = kKeepLines;
    {
        in.setPos(size - 1);
        char last = 0;
        size_t n = 1;
        in.read(&last, n);
        if(n == 1 && last == '\n')
            ++need;
    }

    constexpr size_t CHUNK = 8192;
    char buf[CHUNK];
    int64_t pos = size;
    int found = 0;

    while(pos > 0 && found < need) {
        const size_t n = static_cast<size_t>(std::min<int64_t>(static_cast<int64_t>(CHUNK), pos));
        pos -= static_cast<int64_t>(n);
        in.setPos(pos);
        size_t len = n;
        in.read(buf, len);
        for(size_t i = len; i-- > 0; ) {
            if(buf[i] == '\n') {
                ++found;
                if(found == need)
                    return pos + static_cast<int64_t>(i) + 1;
            }
        }
    }
    return 0;
}

void trimLogFile(const string& path) {
    const int maxMb = SettingsManager::getInstance()->get(SettingsManager::LOG_MAX_FILE_SIZE, true);
    if(maxMb <= 0 || !Util::fileExists(path))
        return;

    try {
        const int64_t maxBytes = static_cast<int64_t>(maxMb) * 1024 * 1024;
        const int64_t size = File::getSize(path);
        if(size <= 0 || size <= maxBytes)
            return;

        File in(path, File::READ, File::OPEN);
        const int64_t skipPos = keepFrom(in, size);
        if(skipPos <= 0)
            return;

        in.setPos(skipPos);
        const string tmp = path + ".trimtmp";
        File out(tmp, File::WRITE, File::OPEN | File::CREATE | File::TRUNCATE);
        copyRest(in, out);
        in.close();
        out.close();
        File::renameFile(tmp, path);
    } catch (const FileException&) {
        File::deleteFile(path + ".trimtmp");
    }
}

void trimLogDir(const string& dir, int depth) {
    if(depth > kMaxDepth || dir.empty())
        return;

    string path = dir;
    if(path.back() != PATH_SEPARATOR)
        path += PATH_SEPARATOR;

    for(const string& p : File::findFiles(path, "*")) {
        if(p.empty())
            continue;
        if(p.back() == PATH_SEPARATOR) {
            const string name = Util::getFileName(p.substr(0, p.size() - 1));
            if(name != "." && name != "..")
                trimLogDir(p, depth + 1);
        } else if(Util::stricmp(Util::getFileExt(p), ".log") == 0) {
            trimLogFile(p);
        }
    }
}

} // namespace

void trimLogFiles() {
    const string dir = SETTING(LOG_DIRECTORY);
    if(dir.empty() || !Util::fileExists(dir))
        return;
    trimLogDir(dir, 0);
}

} // namespace dcpp
