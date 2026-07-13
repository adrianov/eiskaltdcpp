/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "QueueItem.h"

#include "Download.h"
#include "SettingsManager.h"
#include "Util.h"

namespace dcpp {

Segment QueueItem::getNextSegment(int64_t blockSize, int64_t wantedSize, int64_t lastSpeed,
                                  const PartialSource::Ptr partialSource) const {
    if(getSize() == -1 || blockSize == 0) {
        return Segment(0, -1);
    }

    if(!BOOLSETTING(SEGMENTED_DL)) {
        if(!downloads.empty()) {
            return Segment(0, 0);
        }

        int64_t start = 0;
        int64_t end = getSize();

        if(!done.empty()) {
            const Segment& first = *done.begin();

            if(first.getStart() > 0) {
                end = Util::roundUp(first.getStart(), blockSize);
            } else {
                start = Util::roundDown(first.getEnd(), blockSize);

                if(done.size() > 1) {
                    const Segment& second = *(++done.begin());
                    end = Util::roundUp(second.getStart(), blockSize);
                }
            }
        }

        return Segment(start, std::min(getSize(), end) - start);
    }

    /* added for PFS */
    vector<int64_t> posArray;
    vector<Segment> neededParts;

    if(partialSource) {
        posArray.reserve(partialSource->getPartialInfo().size());

        // Convert block index to file position
        for(PartsInfo::const_iterator i = partialSource->getPartialInfo().begin();
            i != partialSource->getPartialInfo().end(); ++i)
            posArray.push_back(min(getSize(), (int64_t)(*i) * blockSize));
    }

    /***************************/

    double donePart = static_cast<double>(getDownloadedBytes()) / getSize();

    // We want smaller blocks at the end of the transfer, squaring gives a nice curve...
    int64_t targetSize = SETTING(SEGMENT_SIZE) > 0 ? (int64_t)(SETTING(SEGMENT_SIZE)*1024*1024)
                                                   : wantedSize * std::max(0.25, (1. - (donePart * donePart)));

    if(targetSize > blockSize) {
        // Round off to nearest block size
        targetSize = Util::roundDown(targetSize, blockSize);
    } else {
        targetSize = blockSize;
    }

    int64_t start = 0;
    int64_t curSize = targetSize;

    while(start < getSize()) {
        const int64_t end = std::min(getSize(), start + curSize);
        Segment block(start, end - start);
        bool overlaps = false;
        for(auto i = done.begin(); !overlaps && i != done.end(); ++i) {
            if(curSize <= blockSize) {
                const int64_t dstart = i->getStart();
                const int64_t dend = i->getEnd();
                // We accept partial overlaps, only consider the block done if it is fully consumed by the done block
                if(dstart <= start && dend >= end) {
                    overlaps = true;
                }
            } else {
                overlaps = block.overlaps(*i);
            }
        }

        for(auto i = downloads.begin(); !overlaps && i != downloads.end(); ++i) {
            overlaps = block.overlaps((*i)->getSegment());
        }

        if(!overlaps) {
            if(partialSource) {
                // store all chunks we could need
                for(vector<int64_t>::const_iterator j = posArray.begin(); j < posArray.end(); j += 2){
                    if( (*j <= start && start < *(j+1)) || (start <= *j && *j < end) ) {
                        int64_t b = max(start, *j);
                        int64_t e = min(end, *(j+1));

                        bool merged = false;
                        if(!neededParts.empty())
                        {
                            Segment& prev = neededParts.back();
                            if(b == prev.getEnd() && e > prev.getEnd())
                            {
                                prev.setSize(prev.getSize() + (e - b));
                                merged = true;
                            }
                        }

                        if(!merged)
                            neededParts.push_back(Segment(b, e - b));
                    }
                }
            } else {
                return block;
            }
        }

        if(!partialSource && curSize > blockSize) {
            curSize -= blockSize;
        } else {
            start = end;
            curSize = targetSize;
        }
    }

    if(!neededParts.empty()) {
        // select random chunk for PFS
        dcdebug("Found partial chunks: %d\n", static_cast<int>(neededParts.size()));

        Segment& selected = neededParts[Util::rand(0, neededParts.size())];
        selected.setSize(std::min(selected.getSize(), targetSize));     // request only wanted size

        return selected;
    }

    if(partialSource == NULL && BOOLSETTING(OVERLAP_CHUNKS) && lastSpeed > 0) {
        // overlap slow running chunk

        for(auto d: downloads) {

            // current chunk mustn't be already overlapped
            if(d->getOverlapped())
                continue;

            // current chunk must be running at least for 2 seconds
            if(d->getStart() == 0 || GET_TIME() - d->getStart() < 2000)
                continue;

            // current chunk mustn't be finished in next 10 seconds
            if(d->getSecondsLeft() < 10)
                continue;

            // overlap current chunk at last block boundary
            const int64_t pos = d->getPos() - (d->getPos() % blockSize);
            const int64_t size = d->getSize() - pos;

            // new user should finish this chunk more than 2x faster
            const int64_t newChunkLeft = size / lastSpeed;
            if(2 * newChunkLeft < d->getSecondsLeft()) {
                return Segment(d->getStartPos() + pos, size/*, true*/); // TODO: bool
            }
        }
    }

    return Segment(0, 0);
}

} // namespace dcpp
