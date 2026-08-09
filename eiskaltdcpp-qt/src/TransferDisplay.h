/***************************************************************************
*                                                                         *
*   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QString>
#include <cstdint>

namespace TransferDisplay {

constexpr int ByteSigFigs = 2;

double roundBytes(double bytes);
inline double roundSpeed(double speed) { return roundBytes(speed); }

/**
 * Time left (seconds): never increase.
 * If the new estimate is lower, step to the midpoint of shown and estimate.
 * If the estimate is unknown, keep the value already shown.
 */
inline int64_t smoothTimeLeft(int64_t shown, int64_t estimate)
{
    if (estimate < 0)
        return shown;
    if (shown < 0)
        return estimate;
    if (estimate < shown)
        return (shown + estimate) / 2;
    return shown;
}

/** True for "Downloaded …" / "Uploaded …" progress status text. */
inline bool isProgressStat(const QString &stat, const QString &downloadedPrefix, const QString &uploadedPrefix)
{
    return stat.startsWith(downloadedPrefix) || stat.startsWith(uploadedPrefix);
}

/** High-water mark: multi-segment/source ticks can briefly under-count. */
inline qlonglong highWaterBytes(qlonglong shown, qlonglong next)
{
    return next < shown ? shown : next;
}

} // namespace TransferDisplay
