/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "QueueItem.h"

#include "SettingsManager.h"
#include "queue/SegmentPicker.h"

namespace dcpp {

Segment QueueItem::getNextSegment(int64_t blockSize, int64_t wantedSize, int64_t lastSpeed,
                                  const PartialSource::Ptr partialSource) const {
    if(getSize() == -1 || blockSize == 0)
        return Segment(0, -1);

    SegmentPicker picker(getSize(), blockSize, done, downloads);
    picker.setVideoTail(getTarget());

    if(!BOOLSETTING(SEGMENTED_DL))
        return picker.nextWhole();

    return picker.nextChunk(wantedSize, getDownloadedBytes(), lastSpeed, partialSource);
}

} // namespace dcpp
