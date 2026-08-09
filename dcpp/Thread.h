/*
 * Copyright (C) 2001-2019 Jacek Sieka, arnetheduck on gmail point com
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

#pragma once

#ifdef _WIN32
#include "w.h"
#else
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#endif

#include <atomic>
#include <cstdint>

#include "NonCopyable.h"
#include "Exception.h"

namespace dcpp {

STANDARD_EXCEPTION(ThreadException);

class Thread : private NonCopyable
{
public:
    enum Priority {
        IDLE,
        LOW,
        NORMAL,
        HIGH
    };

#ifdef _WIN32
    Thread(): threadHandle(INVALID_HANDLE_VALUE), threadId(0){ }
    virtual ~Thread() {
        if(threadHandle != INVALID_HANDLE_VALUE)
            CloseHandle(threadHandle);
    }

    void start();
    void join() {
        if(threadHandle == INVALID_HANDLE_VALUE) {
            return;
        }

        WaitForSingleObject(threadHandle, INFINITE);
        CloseHandle(threadHandle);
        threadHandle = INVALID_HANDLE_VALUE;
    }

    static void sleep(uint32_t millis) { ::Sleep(millis); }
    static void yield() { ::Sleep(0); }

#else
    Thread(): threadHandle(0) { }
    virtual ~Thread() {
        if(threadHandle != 0) {
            pthread_detach(threadHandle);
        }
    }
    void start();
    void join() {
        if (threadHandle) {
            pthread_join(threadHandle, 0);
            threadHandle = 0;
        }
    }

    static void sleep(uint32_t millis) { ::usleep(millis*1000); }
    static void yield() { ::sched_yield(); }

#endif

    /** Store priority; apply now when possible (self, or Win32 handle). */
    void setThreadPriority(Priority p);
    Priority getThreadPriority() const { return threadPriority.load(std::memory_order_relaxed); }

protected:
    virtual int run() = 0;

    /** Apply getThreadPriority() to the calling thread (CPU + I/O class). */
    void applyThreadPriority();

#ifdef _WIN32
    HANDLE threadHandle;
    DWORD threadId;
    static DWORD WINAPI starter(void* p);
#else
    pthread_t threadHandle;
    static void* starter(void* p);
#endif

private:
    std::atomic<Priority> threadPriority{NORMAL};
};

} // namespace dcpp
