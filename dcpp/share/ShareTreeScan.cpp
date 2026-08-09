/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "share/ShareTreeScan.h"

#include "File.h"
#include "HashManager.h"
#include "LogManager.h"
#include "SettingsManager.h"
#include "Util.h"
#include "Wildcards.h"
#include "sharemedia/MediaInfoScan.h"
#include "format.h"

namespace dcpp {

ShareTreeScan::ShareTreeScan() : skipList(SETTING(SKIPLIST_SHARE)) {
}

bool ShareTreeScan::keepEntry(const string& name, FileFindIter& it) const {
    if(name.empty())
        return false;
    if(name == "." || name == "..")
        return false;
    if(!BOOLSETTING(SHARE_HIDDEN) && it->isHidden())
        return false;
    if(!BOOLSETTING(FOLLOW_LINKS) && it->isLink())
        return false;
    return true;
}

bool ShareTreeScan::blockedBySkiplist(const string& fileName, int64_t size) const {
    if(skipList.empty() || !Wildcard::patternMatch(fileName, skipList, '|'))
        return false;
    LogManager::getInstance()->message(str(F_("Skip share file: %1% (Size: %2%)")
            % Util::addBrackets(fileName) % Util::formatBytes(size)));
    return true;
}

bool ShareTreeScan::isReservedDir(const string& dirPath) const {
    return ::strcmp(dirPath.c_str(), SETTING(TEMP_DOWNLOAD_DIRECTORY).c_str()) == 0
            || ::strcmp(dirPath.c_str(), Util::getPath(Util::PATH_USER_CONFIG).c_str()) == 0
            || ::strcmp(dirPath.c_str(), SETTING(LOG_DIRECTORY).c_str()) == 0;
}

bool ShareTreeScan::keepFile(const string& name, const string& fileName, int64_t size) const {
    if(name == "Thumbs.db" || name == "desktop.ini" || name == "folder.htt")
        return false;

    const string ext = Util::getFileExt(name);
    if(!BOOLSETTING(SHARE_TEMP_FILES) && ::strcmp(ext.c_str(), ".dctmp") == 0) {
        LogManager::getInstance()->message(str(F_("Skip share temp file: %1% (Size: %2%)")
                % Util::addBrackets(fileName) % Util::formatBytes(size)));
        return false;
    }
    if(BOOLSETTING(SHARE_SKIP_ZERO_BYTE) && size == 0)
        return false;
    if(Util::stricmp(fileName, SETTING(TLS_PRIVATE_KEY_FILE)) == 0)
        return false;
    return true;
}

void ShareTreeScan::addFile(ShareManager::Directory::Ptr& dir,
                            ShareManager::Directory::File::Set::iterator& lastFile,
                            const string& name, const string& fileName, int64_t size,
                            uint32_t lastWrite) {
    try {
        if(!HashManager::getInstance()->checkTTH(fileName, size, lastWrite))
            return;
        ShareManager::Directory::File f(name, size, dir,
                HashManager::getInstance()->getTTH(fileName, size));
        f.setTS(lastWrite);
        mediaInfoFill(fileName, size, f.getTTH(), f.mediaInfo);
        lastFile = dir->files.insert(lastFile, f);
    } catch(const HashException&) {
    }
}

ShareManager::Directory::Ptr ShareTreeScan::build(const string& path,
                                                  const ShareManager::Directory::Ptr& parent) {
    auto dir = ShareManager::Directory::create(Util::getLastDir(path), parent);
    auto lastFile = dir->files.begin();
    FileFindIter end;

#ifdef _WIN32
    for(FileFindIter i(path + "*"); i != end; ++i) {
#else
    for(FileFindIter i(path); i != end; ++i) {
#endif
        const string name = i->getFileName();
        if(name.empty()) {
            LogManager::getInstance()->message(str(F_("Invalid file name found while hashing folder %1%")
                    % Util::addBrackets(path)));
            continue;
        }
        if(!keepEntry(name, i))
            continue;

        const int64_t size = i->getSize();
        const string fileName = path + name;
        if(blockedBySkiplist(fileName, size))
            continue;

        if(i->isDirectory()) {
            const string childPath = path + name + PATH_SEPARATOR;
            if(!isReservedDir(childPath))
                dir->directories[name] = build(childPath, dir);
            continue;
        }

        if(keepFile(name, fileName, size))
            addFile(dir, lastFile, name, fileName, size, i->getLastWriteTime());
    }
    return dir;
}

} // namespace dcpp
