/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "hash/HashWorker.h"

#include "File.h"
#include "MerkleTree.h"
#include "SettingsManager.h"
#include "Text.h"
#include "TimerManager.h"
#include "Util.h"
#include "ZUtils.h"
#include "hash/StreamStore.h"

#include <algorithm>

#ifndef _WIN32
#include <sys/mman.h>
#include <signal.h>
#include <setjmp.h>
#endif

namespace dcpp {

#ifdef _WIN32
#define BUF_SIZE (256*1024)

bool HashWorker::fastHash(const string& fname, uint8_t* buf, TigerTree& tth, int64_t size, CRC32Filter* xcrc32) {
    HANDLE h = INVALID_HANDLE_VALUE;
    DWORD x, y;
    if (!GetDiskFreeSpaceW(Text::utf8ToWide(Util::getFilePath(fname)).c_str(), &y, &x, &y, &y)) {
        return false;
    } else {
        if ((BUF_SIZE % x) != 0) {
            return false;
        } else {
            h = ::CreateFileW(Text::utf8ToWide(fname).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                              FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL);
            if (h == INVALID_HANDLE_VALUE)
                return false;
        }
    }
    DWORD hn = 0;
    DWORD rn = 0;
    uint8_t* hbuf = buf + BUF_SIZE;
    uint8_t* rbuf = buf;

    OVERLAPPED over = { 0 };
    BOOL res = TRUE;
    over.hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);

    bool ok = false;

    uint64_t lastRead = GET_TICK();
    if (!::ReadFile(h, hbuf, BUF_SIZE, &hn, &over)) {
        if (GetLastError() == ERROR_HANDLE_EOF) {
            hn = 0;
        } else if (GetLastError() == ERROR_IO_PENDING) {
            if (!GetOverlappedResult(h, &over, &hn, TRUE)) {
                if (GetLastError() == ERROR_HANDLE_EOF) {
                    hn = 0;
                } else {
                    goto cleanup;
                }
            }
        } else {
            goto cleanup;
        }
    }

    over.Offset = hn;
    size -= hn;
    while (!stop) {
        if (size > 0) {
            // Start a new overlapped read
            ResetEvent(over.hEvent);
            if (SETTING(MAX_HASH_SPEED) > 0) {
                uint64_t now = GET_TICK();
                uint64_t minTime = hn * 1000LL / (SETTING(MAX_HASH_SPEED) * 1024LL * 1024LL);
                if (lastRead + minTime > now) {
                    uint64_t diff = now - lastRead;
                    Thread::sleep(minTime - diff);
                }
                lastRead = lastRead + minTime;
            } else {
                lastRead = GET_TICK();
            }
            res = ReadFile(h, rbuf, BUF_SIZE, &rn, &over);
        } else {
            rn = 0;
        }

        tth.update(hbuf, hn);
        if (xcrc32)
            (*xcrc32)(hbuf, hn);

        {
            Lock l(cs);
            currentSize = max(currentSize - hn, _LL(0));
        }

        if (size == 0) {
            ok = true;
            break;
        }

        if (!res) {
            // deal with the error code
            switch (GetLastError()) {
            case ERROR_IO_PENDING:
                if (!GetOverlappedResult(h, &over, &rn, TRUE)) {
                    dcdebug("Error 0x%x: %s\n", GetLastError(), Util::translateError(GetLastError()).c_str());
                    goto cleanup;
                }
                break;
            default:
                dcdebug("Error 0x%x: %s\n", GetLastError(), Util::translateError(GetLastError()).c_str());
                goto cleanup;
            }
        }

        instantPause();

        *((uint64_t*)&over.Offset) += rn;
        size -= rn;

        swap(rbuf, hbuf);
        swap(rn, hn);
    }

cleanup:
    ::CloseHandle(over.hEvent);
    ::CloseHandle(h);
    return ok;
}

#else // !_WIN32

static sigjmp_buf sb_env;

#ifndef __HAIKU__
static void sigbus_handler(int signum, siginfo_t* info, void* context)
#else // __HAIKU__
static void sigbus_handler(int signum)
#endif
{
    // Jump back to the fastHash which will return error. Apparently truncating
    // a file in Solaris sets si_code to BUS_OBJERR
#ifndef __HAIKU__
    (void)context;
    if (signum == SIGBUS && (info->si_code == BUS_ADRERR || info->si_code == BUS_OBJERR))
        siglongjmp(sb_env, 1);
#endif
}

bool HashWorker::fastHash(const string& filename, uint8_t* , TigerTree& tth, int64_t size, CRC32Filter* xcrc32) {
    instantPause();

    static StreamStore streamStore;

    if (streamStore.loadTree(filename, tth, -1)){
        printf ("%s: hash [%s] was loaded from Xattr.\n", filename.c_str(), tth.getRoot().toBase32().c_str());
        return true;
    }

    static const int64_t BUF_BYTES = (SETTING(HASH_BUFFER_SIZE_MB) >= 1)? SETTING(HASH_BUFFER_SIZE_MB)*1024*1024 : 0x800000;
    static const int64_t BUF_SIZE = BUF_BYTES - (BUF_BYTES % getpagesize());

    int fd = open(Text::fromUtf8(filename).c_str(), O_RDONLY);
    if(fd == -1) {
        dcdebug("Error opening file %s: %s\n", filename.c_str(), Util::translateError(errno).c_str());
        return false;
    }

    const int maxHashSpeed = SETTING(MAX_HASH_SPEED);
    int64_t pos = 0;
    int64_t size_read = 0;
    void *buf = NULL;
    bool ok = false;

    // Prepare and setup a signal handler in case of SIGBUS during mmapped file reads.
    // SIGBUS can be sent when the file is truncated or in case of read errors.
    struct sigaction act, oldact;
    sigset_t signalset;

    sigemptyset(&signalset);

    act.sa_handler = NULL;
#ifndef __HAIKU__
    act.sa_sigaction = sigbus_handler;
#endif
    act.sa_mask = signalset;
#ifdef SA_SIGINFO
    act.sa_flags = SA_SIGINFO | SA_RESETHAND;
#else
    act.sa_flags = NULL;
#endif
    if (sigaction(SIGBUS, &act, &oldact) == -1) {
        dcdebug("Failed to set signal handler for fastHash\n");
        close(fd);
        return false;   // Better luck with the slow hash.
    }

    uint64_t lastRead = GET_TICK();
    unsigned long mmap_flags = static_cast<bool>(SETTING(HASH_BUFFER_PRIVATE))? MAP_PRIVATE : MAP_SHARED;
#ifdef MAP_POPULATE
    if (static_cast<bool>(SETTING(HASH_BUFFER_POPULATE)))
        mmap_flags |= MAP_POPULATE;
#endif
#ifdef MAP_NORESERVE
    if (static_cast<bool>(SETTING(HASH_BUFFER_NORESERVE)))
        mmap_flags |= MAP_NORESERVE;
#endif
    while (pos < size && !stop) {
        size_read = std::min(size - pos, BUF_SIZE);
        buf = mmap(0, size_read, PROT_READ, mmap_flags, fd, pos);
        if(buf == MAP_FAILED) {
            dcdebug("Error calling mmap for file %s: %s\n", filename.c_str(), Util::translateError(errno).c_str());
            break;
        }

        if (sigsetjmp(sb_env, 1)) {
            dcdebug("Caught SIGBUS for file %s\n", filename.c_str());
            break;
        }

        if (posix_madvise(buf, size_read, POSIX_MADV_SEQUENTIAL | POSIX_MADV_WILLNEED) == -1) {
            dcdebug("Error calling madvise for file %s: %s\n", filename.c_str(), Util::translateError(errno).c_str());
            break;
        }

        if(maxHashSpeed > 0) {
            uint64_t now = GET_TICK();
            uint64_t minTime = size_read * 1000LL / (maxHashSpeed * 1024LL * 1024LL);
            
            if(lastRead + minTime> now) {
                uint64_t diff = now - lastRead;
                Thread::sleep(minTime - diff);
            }
            
            lastRead = lastRead + minTime;
        } else {
            lastRead = GET_TICK();
        }

        tth.update(buf, size_read);
        if(xcrc32)
            (*xcrc32)(buf, size_read);

        {
            Lock l(cs);
            currentSize = max(static_cast<uint64_t>(currentSize - size_read), static_cast<uint64_t>(0));
        }

        if (munmap(buf, size_read) == -1) {
            dcdebug("Error calling munmap for file %s: %s\n", filename.c_str(), Util::translateError(errno).c_str());
            break;
        }

        buf = NULL;
        pos += size_read;

        instantPause();

        if (pos == size) {
            ok = true;
        }
    }

    if (buf != NULL && buf != MAP_FAILED && munmap(buf, size_read) == -1) {
        dcdebug("Error calling munmap for file %s: %s\n", filename.c_str(), Util::translateError(errno).c_str());
    }

    close(fd);

    if (sigaction(SIGBUS, &oldact, NULL) == -1) {
        dcdebug("Failed to reset old signal handler for SIGBUS\n");
    }

    if (ok)
        streamStore.saveTree(filename, tth);

    return ok;
}

#endif // !_WIN32

} // namespace dcpp
