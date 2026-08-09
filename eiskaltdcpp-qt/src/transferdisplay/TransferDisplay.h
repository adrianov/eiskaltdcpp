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

/**
 * Compact size/speed text for the Qt UI (via WulforUtil::formatBytes).
 * Same unit ladder as dcpp::Util::formatBytes. Print whole numbers only.
 * From KB up, round so the ones digit is 0 when the value is ≥ 10
 * (23 → 20, 720 → 720). Bytes below one KB stay exact.
 */
double roundBytes(double bytes);
inline double roundSpeed(double speed) { return roundBytes(speed); }
QString formatBytes(int64_t bytes);

/** Time left (seconds): never rise; on a lower estimate, step halfway there. */
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

inline bool isProgressStat(const QString &stat, const QString &downloadedPrefix, const QString &uploadedPrefix)
{
    return stat.startsWith(downloadedPrefix) || stat.startsWith(uploadedPrefix);
}

inline qlonglong highWaterBytes(qlonglong shown, qlonglong next)
{
    return next < shown ? shown : next;
}

} // namespace TransferDisplay
