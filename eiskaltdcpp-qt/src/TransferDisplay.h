/***************************************************************************
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

inline int64_t smoothTimeLeft(int64_t displayed, int64_t actual)
{
    if (actual < 0 || displayed < 0)
        return actual;
    if (actual < displayed)
        return (displayed + actual) / 2;
    if (actual == displayed)
        return actual;
    if (!displayed || actual * 2 >= displayed * 3)
        return actual;
    return displayed;
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
