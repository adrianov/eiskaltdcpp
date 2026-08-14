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
#include "hash/HashStore.h"

#include "File.h"
#include "LogManager.h"
#include "Util.h"
#include "format.h"
#include "hash/HashDataFile.h"
#include "hash/HashIndexXml.h"

#include <algorithm>
#include <utility>

namespace dcpp {

HashStore::HashStore() : dirty(false) {
    HashDataFile::ensureReady();
}

void HashStore::addFile(const string& aFileName, uint32_t aTimeStamp, const TigerTree& tth, bool aUsed) {
    addTree(tth);
    auto fname = Util::getFileName(aFileName), fpath = Util::getFilePath(aFileName);
    auto& fileList = fileIndex[fpath];
    auto j = find(fileList.begin(), fileList.end(), fname);
    if (j != fileList.end())
        fileList.erase(j);
    fileList.emplace_back(fname, tth.getRoot(), aTimeStamp, aUsed);
    dirty = true;
}

void HashStore::addTree(const TigerTree& tt) noexcept {
    if (treeIndex.find(tt.getRoot()) != treeIndex.end())
        return;
    try {
        File f(HashDataFile::path(), File::READ | File::WRITE, File::OPEN);
        int64_t index = HashDataFile::writeLeaves(f, tt);
        treeIndex.emplace(tt.getRoot(), TreeInfo(tt.getFileSize(), index, tt.getBlockSize()));
        dirty = true;
    } catch (const FileException& e) {
        LogManager::getInstance()->message(str(F_("Error saving hash data: %1%") % e.getError()));
    }
}

bool HashStore::getTree(const TTHValue& root, TigerTree& tt) {
    auto i = treeIndex.find(root);
    if (i == treeIndex.end())
        return false;
    try {
        File f(HashDataFile::path(), File::READ, File::OPEN);
        return HashDataFile::readTree(f, i->second.getIndex(), i->second.getSize(),
                                      i->second.getBlockSize(), root, tt);
    } catch (const Exception&) {
        return false;
    }
}

int64_t HashStore::getBlockSize(const TTHValue& root) const {
    auto i = treeIndex.find(root);
    return i == treeIndex.end() ? 0 : i->second.getBlockSize();
}

bool HashStore::checkTTH(const string& aFileName, int64_t aSize, uint32_t aTimeStamp) {
    auto fname = Util::getFileName(aFileName), fpath = Util::getFilePath(aFileName);
    auto i = fileIndex.find(fpath);
    if (i == fileIndex.end())
        return false;
    auto j = find(i->second.begin(), i->second.end(), fname);
    if (j == i->second.end())
        return false;
    auto ti = treeIndex.find(j->getRoot());
    if (ti == treeIndex.end() || ti->second.getSize() != aSize || j->getTimeStamp() != aTimeStamp) {
        i->second.erase(j);
        dirty = true;
        return false;
    }
    return true;
}

const TTHValue* HashStore::getTTH(const string& aFileName) {
    auto i = fileIndex.find(Util::getFilePath(aFileName));
    if (i == fileIndex.end())
        return nullptr;
    auto j = find(i->second.begin(), i->second.end(), Util::getFileName(aFileName));
    if (j == i->second.end())
        return nullptr;
    j->setUsed(true);
    return &(j->getRoot());
}

void HashStore::rebuild() {
    try {
        decltype(fileIndex) newFileIndex;
        decltype(treeIndex) newTreeIndex;

        for (auto& i : fileIndex) {
            for (auto& j : i.second) {
                if (!j.getUsed())
                    continue;
                auto k = treeIndex.find(j.getRoot());
                if (k != treeIndex.end())
                    newTreeIndex[j.getRoot()] = k->second;
            }
        }

        const auto tmpName = HashDataFile::path() + ".tmp";
        const auto origName = HashDataFile::path();
        HashDataFile::create(tmpName);

        {
            File in(origName, File::READ, File::OPEN);
            File out(tmpName, File::READ | File::WRITE, File::OPEN);
            for (auto i = newTreeIndex.begin(); i != newTreeIndex.end();) {
                TigerTree tree;
                if (HashDataFile::readTree(in, i->second.getIndex(), i->second.getSize(),
                                           i->second.getBlockSize(), i->first, tree)) {
                    i->second.setIndex(HashDataFile::writeLeaves(out, tree));
                    ++i;
                } else {
                    newTreeIndex.erase(i++);
                }
            }
        }

        for (auto& i : fileIndex) {
            auto fi = newFileIndex.emplace(i.first, vector<FileInfo>()).first;
            for (auto& j : i.second) {
                if (newTreeIndex.find(j.getRoot()) != newTreeIndex.end())
                    fi->second.push_back(j);
            }
            if (fi->second.empty())
                newFileIndex.erase(fi);
        }

        File::deleteFile(origName);
        File::renameFile(tmpName, origName);
        treeIndex = newTreeIndex;
        fileIndex = newFileIndex;
        dirty = true;
        save();
    } catch (const Exception& e) {
        LogManager::getInstance()->message(str(F_("Hashing failed: %1%") % e.getError()));
    }
}

void HashStore::renameDir(const string& oldPath, const string& newPath) {
    if (oldPath.empty() || newPath.empty() || oldPath == newPath)
        return;

    string oldP = oldPath;
    string newP = newPath;
    if (oldP.back() != PATH_SEPARATOR)
        oldP += PATH_SEPARATOR;
    if (newP.back() != PATH_SEPARATOR)
        newP += PATH_SEPARATOR;
    if (oldP == newP)
        return;

    unordered_map<string, vector<FileInfo>> moved;
    for (auto i = fileIndex.begin(); i != fileIndex.end(); ) {
        if (Util::strnicmp(i->first, oldP, oldP.length()) == 0) {
            moved[newP + i->first.substr(oldP.length())] = std::move(i->second);
            i = fileIndex.erase(i);
            dirty = true;
        } else {
            ++i;
        }
    }
    for (auto& m : moved) {
        auto& dest = fileIndex[m.first];
        dest.insert(dest.end(), m.second.begin(), m.second.end());
    }
}

void HashStore::load() {
    HashIndexXml::load(*this);
}

void HashStore::save() {
    HashIndexXml::save(*this);
}

} // namespace dcpp
