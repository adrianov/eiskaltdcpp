/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "Segment.h"
#include "../QueueItemSource.h"
#include "../typedefs.h"

#include <set>
#include <vector>

namespace dcpp {

/**
 * Chooses the next download byte range for one queue file: video tail first
 * (container index near EOF), then forward hole-fill, PFS parts, or overlap.
 */
class SegmentPicker {
public:
    typedef std::set<Segment> SegmentSet;

    SegmentPicker(int64_t fileSize, int64_t blockSize,
                  const SegmentSet& done, const DownloadList& downloads);

    /** Enable trailing-chunk priority when path is a video and the setting is on. */
    void setVideoTail(const string& path);

    /** Single-connection mode: one free hole (tail preferred). */
    Segment nextWhole() const;

    /** Segmented mode: next chunk, optional PFS source, optional slow-chunk steal. */
    Segment nextChunk(int64_t wantedSize, int64_t downloadedBytes, int64_t lastSpeed,
                      const QueuePartialSource::Ptr& partialSource) const;

private:
    bool busy(const Segment& block, int64_t start, int64_t end, int64_t curSize) const;
    Segment freeFrom(int64_t regionStart, int64_t chunkSize) const;
    Segment pickPfs(std::vector<Segment>& needed, int64_t chunkSize) const;
    Segment stealSlow(int64_t lastSpeed) const;
    int64_t chunkSize(int64_t wantedSize, int64_t downloadedBytes) const;

    int64_t fileSize;
    int64_t blockSize;
    int64_t tailStart; // >= 0 when video EOF chunk should go first
    const SegmentSet& done;
    const DownloadList& downloads;
};

} // namespace dcpp
