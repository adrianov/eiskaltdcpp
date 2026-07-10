/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "UploadManager.h"

#include "QueueManager.h"
#include "FinishedManager.h"
#include "FilteredFile.h"
#include "File.h"
#include "Upload.h"
#include "UserConnection.h"

namespace dcpp {

int UploadManager::openPartialFile(UserConnection& aSource, const string& aFile, int64_t aStartPos, int64_t aBytes,
                                   InputStream*& is, string& sourceFile, int64_t& start, int64_t& size,
                                   int64_t& fileSize, Transfer::Type& type) {
    if (aFile.compare(0, 4, "TTH/") != 0)
        return 0;

    TTHValue fileHash(aFile.substr(4));
    string target;
    if (!QueueManager::getInstance()->isChunkDownloaded(fileHash, aStartPos, aBytes, target, fileSize)) {
        target = FinishedManager::getInstance()->getTarget(fileHash.toBase32());
        if (target.empty() || !Util::fileExists(target))
            return 0;
    }

    sourceFile = target;
    File* f = nullptr;
    try {
        f = new File(sourceFile, File::READ, File::OPEN | File::SHARED);
        start = aStartPos;
        int64_t sz = f->getSize();
        if (start > sz) {
            aSource.fileNotAvail();
            delete f;
            return -1;
        }
        size = (aBytes == -1) ? sz - start : aBytes;
        fileSize = sz;

        if (size > sz - start) {
            aSource.fileNotAvail();
            delete f;
            return -1;
        }

        f->setPos(start);
        is = f;
        if ((start + size) < sz)
            is = new LimitedInputStream<true>(is, size);

        type = Transfer::TYPE_FILE;
        return 1;
    } catch (const Exception&) {
        aSource.fileNotAvail();
        delete f;
        is = nullptr;
        return -1;
    }
}

} // namespace dcpp
