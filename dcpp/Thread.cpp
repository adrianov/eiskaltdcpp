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

#include "stdinc.h"
#include "Thread.h"

#include "format.h"
#ifndef _WIN32
#include "ProcessExit.h"
#include <sys/resource.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <pthread/qos.h>
#elif defined(__linux__)
#include <sys/syscall.h>
#endif
#endif

namespace dcpp {

namespace {

#if defined(__linux__)
// From <linux/ioprio.h> (not always available in user headers).
constexpr int IoPrioWhoProcess = 1;
constexpr int IoPrioClassBe = 2;
constexpr int IoPrioClassIdle = 3;

constexpr int makeIoPrio(int ioClass, int data) {
    return (ioClass << 13) | data;
}

void setLinuxIoPrio(bool idle) {
    const int prio = makeIoPrio(idle ? IoPrioClassIdle : IoPrioClassBe, 0);
    syscall(SYS_ioprio_set, IoPrioWhoProcess, 0, prio);
}

void setLinuxSched(int policy) {
    sched_param sp = {};
    sched_setscheduler(0, policy, &sp);
}
#endif

} // namespace

#ifdef _WIN32

void Thread::start() {
    join();
    if( (threadHandle = CreateThread(NULL, 0, &starter, this, 0, &threadId)) == NULL) {
        throw ThreadException(_("Unable to create thread"));
    }
    applyThreadPriority();
}

DWORD WINAPI Thread::starter(void* p) {
    Thread* t = (Thread*)p;
    t->applyThreadPriority();
    t->run();
    return 0;
}

void Thread::setThreadPriority(Priority p) {
    threadPriority.store(p, std::memory_order_relaxed);
    if(threadHandle != INVALID_HANDLE_VALUE)
        applyThreadPriority();
}

void Thread::applyThreadPriority() {
    if(threadHandle == INVALID_HANDLE_VALUE)
        return;
    int winPrio = THREAD_PRIORITY_NORMAL;
    switch(threadPriority.load(std::memory_order_relaxed)) {
        case IDLE: winPrio = THREAD_PRIORITY_IDLE; break;
        case LOW: winPrio = THREAD_PRIORITY_BELOW_NORMAL; break;
        case HIGH: winPrio = THREAD_PRIORITY_ABOVE_NORMAL; break;
        case NORMAL:
        default: winPrio = THREAD_PRIORITY_NORMAL; break;
    }
    ::SetThreadPriority(threadHandle, winPrio);
}

#else

void Thread::start() {
    join();
    if(pthread_create(&threadHandle, NULL, &starter, this) != 0) {
        throw ThreadException(_("Unable to create thread"));
    }
}

void* Thread::starter(void* p) {
    blockSigpipeInThread();
    Thread* t = (Thread*)p;
    t->applyThreadPriority();
    t->run();
    return nullptr;
}

void Thread::setThreadPriority(Priority p) {
    threadPriority.store(p, std::memory_order_relaxed);
#ifndef __HAIKU__
    if(threadHandle && pthread_equal(pthread_self(), threadHandle))
        applyThreadPriority();
#endif
}

void Thread::applyThreadPriority() {
#ifdef __HAIKU__
    (void)0;
#else
    const Priority p = threadPriority.load(std::memory_order_relaxed);

#if defined(__APPLE__)
    // Match Transmission macOS backgrounding, scoped to this thread: Darwin BG
    // scheduling + throttled disk I/O, plus the lowest QoS class.
    switch(p) {
        case IDLE:
            pthread_set_qos_class_self_np(QOS_CLASS_BACKGROUND, QOS_MIN_RELATIVE_PRIORITY);
            setpriority(PRIO_DARWIN_THREAD, 0, PRIO_DARWIN_BG);
            setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_THREAD, IOPOL_THROTTLE);
            break;
        case LOW:
            pthread_set_qos_class_self_np(QOS_CLASS_UTILITY, 0);
            setpriority(PRIO_DARWIN_THREAD, 0, 0);
            setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_THREAD, IOPOL_UTILITY);
            break;
        case HIGH:
            pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, 0);
            setpriority(PRIO_DARWIN_THREAD, 0, 0);
            setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_THREAD, IOPOL_DEFAULT);
            break;
        case NORMAL:
        default:
            pthread_set_qos_class_self_np(QOS_CLASS_DEFAULT, 0);
            setpriority(PRIO_DARWIN_THREAD, 0, 0);
            setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_THREAD, IOPOL_DEFAULT);
            break;
    }
#elif defined(__linux__)
    // Transmission uses SCHED_BATCH + idle I/O when backgrounded; hashing asks
    // for the lowest idle class, so IDLE uses SCHED_IDLE. Avoid setpriority —
    // PRIO_PROCESS can alter the whole process.
    switch(p) {
        case IDLE:
            setLinuxSched(SCHED_IDLE);
            setLinuxIoPrio(true);
            break;
        case LOW:
            setLinuxSched(SCHED_BATCH);
            setLinuxIoPrio(false);
            break;
        case HIGH:
        case NORMAL:
        default:
            setLinuxSched(SCHED_OTHER);
            setLinuxIoPrio(false);
            break;
    }
#else
    // Portable fallback: thread niceness where the platform treats pid 0 as self.
    int niceVal = 0;
    switch(p) {
        case IDLE: niceVal = 19; break;
        case LOW: niceVal = 10; break;
        case HIGH: niceVal = -1; break;
        case NORMAL:
        default: niceVal = 0; break;
    }
    setpriority(PRIO_PROCESS, 0, niceVal);
#endif
#endif
}

#endif

} // namespace dcpp
