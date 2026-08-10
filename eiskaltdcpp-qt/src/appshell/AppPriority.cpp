/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#include "AppPriority.h"

#include <QEvent>
#include <QGuiApplication>
#include <QWidget>

#if defined(Q_OS_MAC)
#include <sys/resource.h>
#include <unistd.h>
#elif defined(Q_OS_LINUX)
#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#endif

namespace {

#if defined(Q_OS_LINUX)
constexpr int IoPrioWhoProcess = 1;
constexpr int IoPrioClassBe = 2;
constexpr int IoPrioClassIdle = 3;

constexpr int makeIoPrio(int ioClass, int data) {
    return (ioClass << 13) | data;
}
#endif

void applyOsYield(bool yield) {
#if defined(Q_OS_MAC)
    if (yield) {
        setpriority(PRIO_DARWIN_PROCESS, 0, PRIO_DARWIN_BG);
        setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS, IOPOL_THROTTLE);
    } else {
        setpriority(PRIO_DARWIN_PROCESS, 0, 0);
        setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS, IOPOL_DEFAULT);
    }
#elif defined(Q_OS_LINUX)
    sched_param sp = {};
    sched_setscheduler(0, yield ? SCHED_BATCH : SCHED_OTHER, &sp);
    syscall(SYS_ioprio_set, IoPrioWhoProcess, 0,
            makeIoPrio(yield ? IoPrioClassIdle : IoPrioClassBe, 0));
#elif defined(Q_OS_WIN)
    SetPriorityClass(GetCurrentProcess(),
                     yield ? PROCESS_MODE_BACKGROUND_BEGIN : PROCESS_MODE_BACKGROUND_END);
#else
    Q_UNUSED(yield);
#endif
}

} // namespace

AppPriority::AppPriority(QObject *parent)
    : QObject(parent)
{
    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState) {
        applyState();
    });
}

AppPriority::~AppPriority()
{
    restoreNormal();
}

void AppPriority::trackWindow(QWidget *window) {
    if (window_)
        window_->removeEventFilter(this);
    window_ = window;
    if (window_)
        window_->installEventFilter(this);
    applyState();
}

void AppPriority::setYieldAllowed(bool allowed) {
    if (allowed == yieldAllowed_)
        return;
    yieldAllowed_ = allowed;
    applyState();
}

void AppPriority::restoreNormal() {
    setYielding(false);
}

bool AppPriority::eventFilter(QObject *obj, QEvent *event) {
    if (obj == window_ && event->type() == QEvent::WindowStateChange)
        scheduleUpdate();
    return QObject::eventFilter(obj, event);
}

void AppPriority::scheduleUpdate() {
    if (pending_)
        return;
    pending_ = true;
    QMetaObject::invokeMethod(this, &AppPriority::flushUpdate, Qt::QueuedConnection);
}

void AppPriority::flushUpdate() {
    pending_ = false;
    applyState();
}

void AppPriority::applyState() {
    setYielding(shouldYield());
}

bool AppPriority::shouldYield() const {
    if (!yieldAllowed_)
        return false;
    if (qApp->applicationState() != Qt::ApplicationActive)
        return true;
    return window_ && window_->isMinimized();
}

void AppPriority::setYielding(bool on) {
    if (on == yielding_)
        return;
    applyOsYield(on);
    yielding_ = on;
}
