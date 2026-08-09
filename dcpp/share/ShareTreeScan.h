/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "../ShareManager.h"

namespace dcpp {

class FileFindIter;

/**
 * Walks a real filesystem path and builds an in-memory share Directory tree
 * (skiplist, hidden/link filters, TTH gate, media attrs).
 */
class ShareTreeScan {
public:
    ShareTreeScan();

    ShareManager::Directory::Ptr build(const string& path,
                                       const ShareManager::Directory::Ptr& parent);

private:
    string skipList;

    bool keepEntry(const string& name, FileFindIter& it) const;
    bool blockedBySkiplist(const string& fileName, int64_t size) const;
    bool isReservedDir(const string& dirPath) const;
    bool keepFile(const string& name, const string& fileName, int64_t size) const;
    void addFile(ShareManager::Directory::Ptr& dir,
                 ShareManager::Directory::File::Set::iterator& lastFile,
                 const string& name, const string& fileName, int64_t size,
                 uint32_t lastWrite);
};

} // namespace dcpp
