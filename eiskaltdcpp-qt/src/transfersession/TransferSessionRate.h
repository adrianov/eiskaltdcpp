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

#include <algorithm>
#include <cstdint>

/**
 * Mean throughput since session begin (see TransferSessionRate.md).
 *
 *   rate      = moved / elapsed
 *   progress  = baseline + moved
 *   remaining = fileSize - progress
 *   eta       = remaining / rate
 */
namespace TransferSessionRate {

constexpr uint64_t MinElapsedMs = 1000;

struct Input {
    int64_t moved = 0;     /**< Bytes transferred after baseline. */
    int64_t baseline = 0;  /**< File bytes already done at session begin. */
    int64_t fileSize = 0;
    uint64_t startTick = 0;
    uint64_t nowTick = 0;
};

struct Result {
    double bytesPerSec = 0.0;
    int64_t etaSec = -1;
    int64_t progress = 0;
    int64_t remaining = 0;
};

inline Result compute(const Input &in)
{
    Result out;
    const int64_t moved = std::max<int64_t>(0, in.moved);
    const int64_t baseline = std::max<int64_t>(0, in.baseline);
    int64_t progress = baseline + moved;
    if (in.fileSize > 0)
        progress = std::min(progress, in.fileSize);
    out.progress = progress;
    out.remaining = (in.fileSize > progress) ? (in.fileSize - progress) : 0;

    if (in.startTick == 0 || in.nowTick < in.startTick || moved <= 0)
        return out;

    const uint64_t elapsed = in.nowTick - in.startTick;
    if (elapsed < MinElapsedMs)
        return out;

    out.bytesPerSec = (static_cast<double>(moved) * 1000.0)
            / static_cast<double>(elapsed);
    if (out.bytesPerSec > 0.0 && out.remaining > 0)
        out.etaSec = static_cast<int64_t>(out.remaining / out.bytesPerSec);
    return out;
}

} // namespace TransferSessionRate
