/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "queue/SegmentPicker.h"

#include "Download.h"
#include "SettingsManager.h"
#include "Text.h"
#include "Util.h"

namespace dcpp {

namespace {

bool isVideoFile(const string& path) {
    // Containers that often keep the index near EOF (ADC video search set + common tails).
    static const char* exts[] = {
        "3gp", "asf", "asx", "avi", "divx", "flv", "m2ts", "m4v", "mkv", "mov",
        "mp4", "mpeg", "mpg", "ogm", "pxp", "qt", "rm", "rmvb", "swf", "ts",
        "vob", "webm", "wmv", nullptr
    };
    string e = Util::getFileExt(path);
    if(!e.empty() && e[0] == '.')
        e.erase(0, 1);
    e = Text::toLower(e);
    for(const char** p = exts; *p; ++p) {
        if(e == *p)
            return true;
    }
    return false;
}

} // namespace

SegmentPicker::SegmentPicker(int64_t fileSize_, int64_t blockSize_,
                             const SegmentSet& done_, const DownloadList& downloads_) :
    fileSize(fileSize_), blockSize(blockSize_), tailStart(-1), done(done_), downloads(downloads_)
{ }

void SegmentPicker::setVideoTail(const string& path) {
    tailStart = -1;
    if(!BOOLSETTING(VIDEO_END_FIRST) || !isVideoFile(path) || blockSize <= 0)
        return;

    int64_t endBytes = (int64_t)SETTING(VIDEO_END_FIRST_SIZE) * 1024 * 1024;
    if(endBytes <= 0)
        endBytes = 20 * 1024 * 1024;
    if(fileSize <= endBytes)
        return;

    tailStart = Util::roundDown(fileSize - endBytes, blockSize);
}

bool SegmentPicker::busy(const Segment& block, int64_t start, int64_t end, int64_t curSize) const {
    for(auto i = done.begin(); i != done.end(); ++i) {
        if(curSize <= blockSize) {
            if(i->getStart() <= start && i->getEnd() >= end)
                return true;
        } else if(block.overlaps(*i)) {
            return true;
        }
    }
    for(auto i = downloads.begin(); i != downloads.end(); ++i) {
        if(block.overlaps((*i)->getSegment()))
            return true;
    }
    return false;
}

Segment SegmentPicker::freeFrom(int64_t regionStart, int64_t chunkSize) const {
    int64_t start = regionStart;
    int64_t curSize = chunkSize;
    while(start < fileSize) {
        const int64_t end = std::min(fileSize, start + curSize);
        Segment block(start, end - start);
        if(!busy(block, start, end, curSize))
            return block;

        if(curSize > blockSize) {
            curSize -= blockSize;
        } else {
            start = end;
            curSize = chunkSize;
        }
    }
    return Segment(0, 0);
}

int64_t SegmentPicker::chunkSize(int64_t wantedSize, int64_t downloadedBytes) const {
    const double donePart = static_cast<double>(downloadedBytes) / fileSize;
    int64_t target = SETTING(SEGMENT_SIZE) > 0
                     ? (int64_t)(SETTING(SEGMENT_SIZE) * 1024 * 1024)
                     : wantedSize * std::max(0.25, (1. - (donePart * donePart)));

    if(target > blockSize)
        return Util::roundDown(target, blockSize);
    return blockSize;
}

Segment SegmentPicker::nextWhole() const {
    if(!downloads.empty())
        return Segment(0, 0);

    if(tailStart >= 0) {
        Segment endSeg = freeFrom(tailStart, fileSize - tailStart);
        if(endSeg.getSize() > 0)
            return endSeg;
    }

    int64_t start = 0;
    int64_t end = fileSize;
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
    return Segment(start, std::min(fileSize, end) - start);
}

Segment SegmentPicker::pickPfs(std::vector<Segment>& needed, int64_t target) const {
    if(tailStart >= 0) {
        for(size_t i = 0; i < needed.size(); ++i) {
            Segment& part = needed[i];
            if(part.getEnd() <= tailStart)
                continue;
            if(part.getStart() < tailStart) {
                const int64_t b = tailStart;
                const int64_t e = part.getEnd();
                return Segment(b, std::min(e - b, target));
            }
            part.setSize(std::min(part.getSize(), target));
            return part;
        }
    }

    dcdebug("Found partial chunks: %d\n", static_cast<int>(needed.size()));
    Segment& selected = needed[Util::rand(0, needed.size())];
    selected.setSize(std::min(selected.getSize(), target));
    return selected;
}

Segment SegmentPicker::stealSlow(int64_t lastSpeed) const {
    if(!BOOLSETTING(OVERLAP_CHUNKS) || lastSpeed <= 0)
        return Segment(0, 0);

    for(auto d: downloads) {
        if(d->getOverlapped())
            continue;
        if(d->getStart() == 0 || GET_TIME() - d->getStart() < 2000)
            continue;
        if(d->getSecondsLeft() < 10)
            continue;

        const int64_t pos = d->getPos() - (d->getPos() % blockSize);
        const int64_t size = d->getSize() - pos;
        const int64_t newChunkLeft = size / lastSpeed;
        if(2 * newChunkLeft < d->getSecondsLeft())
            return Segment(d->getStartPos() + pos, size);
    }
    return Segment(0, 0);
}

Segment SegmentPicker::nextChunk(int64_t wantedSize, int64_t downloadedBytes, int64_t lastSpeed,
                                 const QueuePartialSource::Ptr& partialSource) const {
    std::vector<int64_t> posArray;
    std::vector<Segment> neededParts;

    if(partialSource) {
        posArray.reserve(partialSource->getPartialInfo().size());
        for(PartsInfo::const_iterator i = partialSource->getPartialInfo().begin();
            i != partialSource->getPartialInfo().end(); ++i)
            posArray.push_back(min(fileSize, (int64_t)(*i) * blockSize));
    }

    const int64_t target = chunkSize(wantedSize, downloadedBytes);

    if(tailStart >= 0 && !partialSource) {
        Segment endSeg = freeFrom(tailStart, target);
        if(endSeg.getSize() > 0)
            return endSeg;
    }

    int64_t start = 0;
    int64_t curSize = target;
    while(start < fileSize) {
        const int64_t end = std::min(fileSize, start + curSize);
        Segment block(start, end - start);
        if(!busy(block, start, end, curSize)) {
            if(partialSource) {
                for(std::vector<int64_t>::const_iterator j = posArray.begin(); j < posArray.end(); j += 2) {
                    if((*j <= start && start < *(j+1)) || (start <= *j && *j < end)) {
                        int64_t b = max(start, *j);
                        int64_t e = min(end, *(j+1));
                        bool merged = false;
                        if(!neededParts.empty()) {
                            Segment& prev = neededParts.back();
                            if(b == prev.getEnd() && e > prev.getEnd()) {
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
            curSize = target;
        }
    }

    if(!neededParts.empty())
        return pickPfs(neededParts, target);

    if(!partialSource)
        return stealSlow(lastSpeed);

    return Segment(0, 0);
}

} // namespace dcpp
