/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "stdinc.h"
#include "hash/HashFileGate.h"

#include "CriticalSection.h"
#include "File.h"
#include "Text.h"
#include "TimerManager.h"

#include <unordered_set>

#ifdef _WIN32
#include "w.h"
#else
#include <climits>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#ifdef __APPLE__
#include <libproc.h>
#include <sys/proc_info.h>
#elif defined(__linux__)
#include <cstdio>
#include <dirent.h>
#include <cctype>
#endif
#endif

namespace dcpp {

namespace {

#ifdef _WIN32

bool winWriterOpen(const string& path) {
    HANDLE h = ::CreateFileW(Text::utf8ToWide(path).c_str(), GENERIC_READ, FILE_SHARE_READ,
                             nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return ::GetLastError() == ERROR_SHARING_VIOLATION;
    ::CloseHandle(h);
    return false;
}

bool winMeta(const string& path, int64_t& size, uint32_t& mtime) {
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!::GetFileAttributesExW(Text::utf8ToWide(path).c_str(), GetFileExInfoStandard, &data))
        return false;
    ULARGE_INTEGER u;
    u.LowPart = data.nFileSizeLow;
    u.HighPart = data.nFileSizeHigh;
    size = static_cast<int64_t>(u.QuadPart);
    mtime = File::convertTime(&data.ftLastWriteTime);
    return true;
}

#else // !_WIN32

string nativePath(const string& utf8) {
    return Text::fromUtf8(utf8);
}

string realPath(const string& native) {
    char buf[PATH_MAX];
    if (::realpath(native.c_str(), buf))
        return string(buf);
    return native;
}

bool unixMeta(const string& path, int64_t& size, uint32_t& mtime) {
    struct stat s;
    if (::stat(nativePath(path).c_str(), &s) == -1)
        return false;
    size = static_cast<int64_t>(s.st_size);
    mtime = static_cast<uint32_t>(s.st_mtime);
    return true;
}

#if defined(__APPLE__) || defined(__linux__)

CriticalSection cacheCs;
unordered_set<string> writePaths;
uint64_t cacheTick = 0;
const uint64_t CACHE_MS = 3000;

#ifdef __APPLE__

void scanWriters(unordered_set<string>& out) {
    int bytes = proc_listpids(PROC_ALL_PIDS, 0, nullptr, 0);
    if (bytes <= 0)
        return;
    vector<pid_t> pids(static_cast<size_t>(bytes) / sizeof(pid_t));
    bytes = proc_listpids(PROC_ALL_PIDS, 0, pids.data(), bytes);
    if (bytes <= 0)
        return;
    pids.resize(static_cast<size_t>(bytes) / sizeof(pid_t));

    vector<proc_fdinfo> fds;
    for (pid_t pid : pids) {
        if (pid <= 0)
            continue;
        int fdBytes = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, nullptr, 0);
        if (fdBytes <= 0)
            continue;
        fds.resize(static_cast<size_t>(fdBytes) / sizeof(proc_fdinfo));
        fdBytes = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, fds.data(), fdBytes);
        if (fdBytes <= 0)
            continue;
        const size_t n = static_cast<size_t>(fdBytes) / sizeof(proc_fdinfo);
        for (size_t i = 0; i < n; ++i) {
            if (fds[i].proc_fdtype != PROX_FDTYPE_VNODE)
                continue;
            vnode_fdinfowithpath info;
            int got = proc_pidfdinfo(pid, fds[i].proc_fd, PROC_PIDFDVNODEPATHINFO, &info, sizeof(info));
            if (got < static_cast<int>(sizeof(info)))
                continue;
            if ((info.pfi.fi_openflags & O_ACCMODE) == O_RDONLY)
                continue;
            if (info.pvip.vip_path[0] == '\0')
                continue;
            out.insert(realPath(string(info.pvip.vip_path)));
        }
    }
}

#elif defined(__linux__)

bool fdWritable(const string& fdinfoPath) {
    FILE* f = ::fopen(fdinfoPath.c_str(), "r");
    if (!f)
        return false;
    char line[256];
    bool write = false;
    while (::fgets(line, sizeof(line), f)) {
        unsigned long flags = 0;
        if (::sscanf(line, "flags:\t%lo", &flags) == 1 || ::sscanf(line, "flags: %lo", &flags) == 1) {
            write = (flags & O_ACCMODE) != O_RDONLY;
            break;
        }
    }
    ::fclose(f);
    return write;
}

void scanWriters(unordered_set<string>& out) {
    DIR* proc = ::opendir("/proc");
    if (!proc)
        return;
    while (dirent* pe = ::readdir(proc)) {
        if (!std::isdigit(static_cast<unsigned char>(pe->d_name[0])))
            continue;
        string fdDir = string("/proc/") + pe->d_name + "/fd";
        DIR* fd = ::opendir(fdDir.c_str());
        if (!fd)
            continue;
        while (dirent* fe = ::readdir(fd)) {
            if (fe->d_name[0] == '.')
                continue;
            string linkPath = fdDir + '/' + fe->d_name;
            char target[PATH_MAX];
            ssize_t n = ::readlink(linkPath.c_str(), target, sizeof(target) - 1);
            if (n <= 0)
                continue;
            target[n] = '\0';
            if (target[0] != '/')
                continue;
            if (!fdWritable(string("/proc/") + pe->d_name + "/fdinfo/" + fe->d_name))
                continue;
            out.insert(realPath(string(target)));
        }
        ::closedir(fd);
    }
    ::closedir(proc);
}

#endif

bool unixWriterOpen(const string& path) {
    const string key = realPath(nativePath(path));
    Lock l(cacheCs);
    const uint64_t now = GET_TICK();
    if (cacheTick == 0 || now - cacheTick >= CACHE_MS) {
        writePaths.clear();
        scanWriters(writePaths);
        cacheTick = now;
    }
    return writePaths.find(key) != writePaths.end();
}

#else // other Unix

bool unixWriterOpen(const string&) {
    return false;
}

#endif

#endif // !_WIN32

} // namespace

bool HashFileGate::writerOpen(const string& path) noexcept {
    if (path.empty())
        return false;
#ifdef _WIN32
    return winWriterOpen(path);
#else
    return unixWriterOpen(path);
#endif
}

bool HashFileGate::metaStable(const string& path, int64_t size, uint32_t mtime) noexcept {
    int64_t curSize = -1;
    uint32_t curMtime = 0;
#ifdef _WIN32
    if (!winMeta(path, curSize, curMtime))
        return false;
#else
    if (!unixMeta(path, curSize, curMtime))
        return false;
#endif
    return curSize == size && curMtime == mtime;
}

} // namespace dcpp
