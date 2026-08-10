/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <QObject>
#include <QPointer>

class QEvent;
class QWidget;

/**
 * Yields process CPU/disk while inactive or minimized (Transmission-style
 * Darwin BG + disk throttle). Yield stays off until setYieldAllowed(true)
 * so cold start always runs at normal priority.
 */
class AppPriority : public QObject {
    Q_OBJECT
public:
    explicit AppPriority(QObject *parent = nullptr);
    ~AppPriority() override;

    void trackWindow(QWidget *window);
    /** When false (default), keep normal scheduling regardless of focus. */
    void setYieldAllowed(bool allowed);
    void restoreNormal();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private Q_SLOTS:
    void flushUpdate();

private:
    void scheduleUpdate();
    void applyState();
    bool shouldYield() const;
    void setYielding(bool on);

    QPointer<QWidget> window_;
    bool pending_ = false;
    bool yielding_ = false;
    bool yieldAllowed_ = false;
};
