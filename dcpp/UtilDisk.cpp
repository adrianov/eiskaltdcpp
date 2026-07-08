/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "Util.h"
#include "format.h"

#ifndef _WIN32
#include <sys/statvfs.h>
#endif

namespace dcpp {

const char Util::NO_SPACE_TAG[] = "ENOSPC";

string Util::noSpaceError() {
    return string(NO_SPACE_TAG) + ": " + _("Not enough disk space");
}

bool Util::isNoSpaceError(int err) {
#ifdef _WIN32
    return err == ERROR_DISK_FULL || err == ERROR_HANDLE_DISK_FULL;
#else
    return err == ENOSPC || err == EDQUOT;
#endif
}

bool Util::isNoSpaceMessage(const string& msg) {
    return msg.find(NO_SPACE_TAG) != string::npos;
}

bool Util::getFreeBytes(const string& path, uint64_t& out) {
    const string dir = getFilePath(path);
#ifdef _WIN32
    ULARGE_INTEGER avail;
    if(GetDiskFreeSpaceExW(Text::utf8ToWide(dir).c_str(), &avail, nullptr, nullptr)) {
        out = avail.QuadPart;
        return true;
    }
#else
    struct statvfs vfs;
    if(statvfs(dir.c_str(), &vfs) == 0) {
        out = static_cast<uint64_t>(vfs.f_bavail) * static_cast<uint64_t>(vfs.f_frsize);
        return true;
    }
#endif
    return false;
}

} // namespace dcpp
