/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "UserConnection.h"

namespace dcpp {

static const int64_t SEGMENT_TIME = 120*1000;
static const int64_t MIN_CHUNK_SIZE = 64*1024;

void UserConnection::updateChunkSize(int64_t leafSize, int64_t lastChunk, uint64_t ticks) {

    if(chunkSize == 0) {
        chunkSize = std::max(MIN_CHUNK_SIZE, std::min(lastChunk, (int64_t)1024*1024));
        return;
    }

    if(ticks <= 10) {
        chunkSize *= 2;
        return;
    }

    double lastSpeed = (1000. * lastChunk) / ticks;

    int64_t targetSize = chunkSize;

    double msecs = 1000 * targetSize / lastSpeed;

    if(msecs < SEGMENT_TIME / 4) {
        targetSize *= 2;
    } else if(msecs < SEGMENT_TIME / 1.25) {
        targetSize += leafSize;
    } else if(msecs < SEGMENT_TIME * 1.25) {
    } else if(msecs < SEGMENT_TIME * 4) {
        targetSize = std::max(MIN_CHUNK_SIZE, targetSize - chunkSize);
    } else {
        targetSize = std::max(MIN_CHUNK_SIZE, targetSize / 2);
    }

    chunkSize = targetSize;
}

} // namespace dcpp
