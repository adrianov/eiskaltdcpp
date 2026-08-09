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
#include "HashManager.h"
#include "LogManager.h"
#include "SimpleXML.h"
#include "Streams.h"
#include "Util.h"
#include "ZUtils.h"

#include <memory>
#include <algorithm>

namespace dcpp {

#define HASH_FILE_VERSION_STRING "2"
static const uint32_t HASH_FILE_VERSION = 2;

void HashStore::addFile(const string& aFileName, uint32_t aTimeStamp, const TigerTree& tth, bool aUsed) {
    addTree(tth);

    auto fname = Util::getFileName(aFileName), fpath = Util::getFilePath(aFileName);

    auto& fileList = fileIndex[fpath];

    auto j = find(fileList.begin(), fileList.end(), fname);
    if (j != fileList.end()) {
        fileList.erase(j);
    }

    fileList.emplace_back(fname, tth.getRoot(), aTimeStamp, aUsed);
    dirty = true;
}

void HashStore::addTree(const TigerTree& tt) noexcept {
    if (treeIndex.find(tt.getRoot()) == treeIndex.end()) {
        try {
            File f(getDataFile(), File::READ | File::WRITE, File::OPEN);
            int64_t index = saveTree(f, tt);
            treeIndex.emplace(tt.getRoot(), TreeInfo(tt.getFileSize(), index, tt.getBlockSize()));
            dirty = true;
        } catch (const FileException& e) {
            LogManager::getInstance()->message(str(F_("Error saving hash data: %1%") % e.getError()));
        }
    }
}

int64_t HashStore::saveTree(File& f, const TigerTree& tt) {
    if (tt.getLeaves().size() == 1)
        return SMALL_TREE;

    f.setPos(0);
    int64_t pos = 0;
    size_t n = sizeof(pos);
    if (f.read(&pos, n) != sizeof(pos))
        throw HashException(_("Unable to read hash data file"));

    // Check if we should grow the file, we grow by a meg at a time...
    int64_t datsz = f.getSize();
    if ((pos + (int64_t) (tt.getLeaves().size() * TTHValue::BYTES)) >= datsz) {
        f.setPos(datsz + 1024 * 1024);
        f.setEOF();
    }
    f.setPos(pos);dcassert(tt.getLeaves().size()> 1);
    f.write(tt.getLeaves()[0].data, (tt.getLeaves().size() * TTHValue::BYTES));
    int64_t p2 = f.getPos();
    f.setPos(0);
    f.write(&p2, sizeof(p2));
    return pos;
}

bool HashStore::loadTree(File& f, const TreeInfo& ti, const TTHValue& root, TigerTree& tt) {
    if (ti.getIndex() == SMALL_TREE) {
        tt = TigerTree(ti.getSize(), ti.getBlockSize(), root);
        return true;
    }
    try {
        f.setPos(ti.getIndex());
        size_t datalen = TigerTree::calcBlocks(ti.getSize(), ti.getBlockSize()) * TTHValue::BYTES;
        std::unique_ptr<uint8_t[]> buf(new uint8_t[datalen]);
        f.read(&buf[0], datalen);
        tt = TigerTree(ti.getSize(), ti.getBlockSize(), &buf[0]);
        if (!(tt.getRoot() == root))
            return false;
    } catch (const Exception&) {
        return false;
    }

    return true;
}

bool HashStore::getTree(const TTHValue& root, TigerTree& tt) {
    auto i = treeIndex.find(root);
    if (i == treeIndex.end())
        return false;
    try {
        File f(getDataFile(), File::READ, File::OPEN);
        return loadTree(f, i->second, root, tt);
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
    if (i != fileIndex.end()) {
        auto j = find(i->second.begin(), i->second.end(), fname);
        if (j != i->second.end()) {
            FileInfo& fi = *j;
            auto ti = treeIndex.find(fi.getRoot());
            if (ti == treeIndex.end() || ti->second.getSize() != aSize || fi.getTimeStamp() != aTimeStamp) {
                i->second.erase(j);
                dirty = true;
                return false;
            }
            return true;
        }
    }
    return false;
}

const TTHValue* HashStore::getTTH(const string& aFileName) {
    string fname = Util::getFileName(aFileName);
    string fpath = Util::getFilePath(aFileName);

    auto i = fileIndex.find(fpath);
    if (i != fileIndex.end()) {
        auto j = find(i->second.begin(), i->second.end(), fname);
        if (j != i->second.end()) {
            j->setUsed(true);
            return &(j->getRoot());
        }
    }
    return NULL;
}

void HashStore::rebuild() {
    try {
        decltype(fileIndex) newFileIndex;
        decltype(treeIndex) newTreeIndex;

        for (auto& i: fileIndex) {
            for (auto& j : i.second) {
                if (!j.getUsed())
                    continue;

                auto k = treeIndex.find(j.getRoot());
                if (k != treeIndex.end()) {
                    newTreeIndex[j.getRoot()] = k->second;
                }
            }
        }

        auto tmpName = getDataFile() + ".tmp";
        auto origName = getDataFile();

        createDataFile(tmpName);

        {
            File in(origName, File::READ, File::OPEN);
            File out(tmpName, File::READ | File::WRITE, File::OPEN);

            for (auto i = newTreeIndex.begin(); i != newTreeIndex.end();) {
                TigerTree tree;
                if (loadTree(in, i->second, i->first, tree)) {
                    i->second.setIndex(saveTree(out, tree));
                    ++i;
                } else {
                    newTreeIndex.erase(i++);
                }
            }
        }

        for (auto& i: fileIndex) {
            auto fi = newFileIndex.emplace(i.first, vector<FileInfo>()).first;

            for (auto& j: i.second) {
                if (newTreeIndex.find(j.getRoot()) != newTreeIndex.end()) {
                    fi->second.push_back(j);
                }
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

void HashStore::save() {
    if (dirty) {
        try {
            File ff(getIndexFile() + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
            BufferedOutputStream<false> f(&ff);

            string tmp;
            string b32tmp;

            f.write(SimpleXML::utf8Header);
            f.write(LIT("<HashStore Version=\"" HASH_FILE_VERSION_STRING "\">\r\n"));

            f.write(LIT("\t<Trees>\r\n"));

            for (auto& i: treeIndex) {
                const TreeInfo& ti = i.second;
                f.write(LIT("\t\t<Hash Type=\"TTH\" Index=\""));
                f.write(Util::toString(ti.getIndex()));
                f.write(LIT("\" BlockSize=\""));
                f.write(Util::toString(ti.getBlockSize()));
                f.write(LIT("\" Size=\""));
                f.write(Util::toString(ti.getSize()));
                f.write(LIT("\" Root=\""));
                b32tmp.clear();
                f.write(i.first.toBase32(b32tmp));
                f.write(LIT("\"/>\r\n"));
            }

            f.write(LIT("\t</Trees>\r\n\t<Files>\r\n"));

            for (auto& i: fileIndex) {
                const string& dir = i.first;
                for (auto& fi: i.second) {
                    f.write(LIT("\t\t<File Name=\""));
                    f.write(SimpleXML::escape(dir + fi.getFileName(), tmp, true));
                    f.write(LIT("\" TimeStamp=\""));
                    f.write(Util::toString(fi.getTimeStamp()));
                    f.write(LIT("\" Root=\""));
                    b32tmp.clear();
                    f.write(fi.getRoot().toBase32(b32tmp));
                    f.write(LIT("\"/>\r\n"));
                }
            }
            f.write(LIT("\t</Files>\r\n</HashStore>"));
            f.flush();
            ff.close();
            File::deleteFile( getIndexFile());
            File::renameFile(getIndexFile() + ".tmp", getIndexFile());

            dirty = false;
        } catch (const FileException& e) {
            LogManager::getInstance()->message(str(F_("Error saving hash data: %1%") % e.getError()));
        }
    }
}

string HashStore::getIndexFile() { return Util::getPath(Util::PATH_USER_CONFIG) + "HashIndex.xml"; }
string HashStore::getDataFile() { return Util::getPath(Util::PATH_USER_CONFIG) + "HashData.dat"; }

class HashLoader: public SimpleXMLReader::CallBack {
public:
    HashLoader(HashStore& s) :
        store(s),
        size(0),
        timeStamp(0),
        version(HASH_FILE_VERSION),
        inTrees(false),
        inFiles(false),
        inHashStore(false)
    { }
    void startTag(const string& name, StringPairList& attribs, bool simple);

private:
    HashStore& store;

    string file;
    int64_t size;
    uint32_t timeStamp;
    int version;

    bool inTrees;
    bool inFiles;
    bool inHashStore;
};

void HashStore::load() {
    try {
        Util::migrate(getIndexFile());

        HashLoader l(*this);
        File f(getIndexFile(), File::READ, File::OPEN);
        SimpleXMLReader(&l).parse(f);
    } catch (const Exception&) {
        // ...
    }
}

static const string sHashStore = "HashStore";
static const string sversion = "version"; // Oops, v1 was like this
static const string sVersion = "Version";
static const string sTrees = "Trees";
static const string sFiles = "Files";
static const string sFile = "File";
static const string sName = "Name";
static const string sSize = "Size";
static const string sHash = "Hash";
static const string sType = "Type";
static const string sTTH = "TTH";
static const string sIndex = "Index";
static const string sLeafSize = "LeafSize"; // Residue from v1 as well
static const string sBlockSize = "BlockSize";
static const string sTimeStamp = "TimeStamp";
static const string sRoot = "Root";

void HashLoader::startTag(const string& name, StringPairList& attribs, bool simple) {
    if (!inHashStore && name == sHashStore) {
        version = Util::toInt(getAttrib(attribs, sVersion, 0));
        if (version == 0) {
            version = Util::toInt(getAttrib(attribs, sversion, 0));
        }
        inHashStore = !simple;
    } else if (inHashStore && version == 2) {
        if (inTrees && name == sHash) {
            const string& type = getAttrib(attribs, sType, 0);
            int64_t index = Util::toInt64(getAttrib(attribs, sIndex, 1));
            int64_t blockSize = Util::toInt64(getAttrib(attribs, sBlockSize, 2));
            int64_t size = Util::toInt64(getAttrib(attribs, sSize, 3));
            const string& root = getAttrib(attribs, sRoot, 4);
            if (!root.empty() && type == sTTH && (index >= 8 || index == HashStore::SMALL_TREE) && blockSize >= 1024) {
                store.treeIndex[TTHValue(root)] = HashStore::TreeInfo(size, index, blockSize);
            }
        } else if (inFiles && name == sFile) {
            file = getAttrib(attribs, sName, 0);
            timeStamp = Util::toUInt32(getAttrib(attribs, sTimeStamp, 1));
            const string& root = getAttrib(attribs, sRoot, 2);

            if (!file.empty() && size >= 0 && timeStamp > 0 && !root.empty()) {
                string fname = Util::getFileName(file);
                string fpath = Util::getFilePath(file);

                store.fileIndex[fpath].emplace_back(fname, TTHValue(root), timeStamp, false);
            }
        } else if (name == sTrees) {
            inTrees = !simple;
        } else if (name == sFiles) {
            inFiles = !simple;
        }
    }
}

HashStore::HashStore() :
    dirty(false) {

    Util::migrate(getDataFile());

    if (File::getSize(getDataFile()) <= static_cast<int64_t> (sizeof(int64_t))) {
        try {
            createDataFile( getDataFile());
        } catch (const FileException&) {
            // ?
        }
    }
}

/**
 * Creates the data files for storing hash values.
 * The data file is very simple in its format. The first 8 bytes
 * are filled with an int64_t (little endian) of the next write position
 * in the file counting from the start (so that file can be grown in chunks).
 * We start with a 1 mb file, and then grow it as needed to avoid fragmentation.
 * To find data inside the file, use the corresponding index file.
 * Since file is never deleted, space will eventually be wasted, so a rebuild
 * should occasionally be done.
 */
void HashStore::createDataFile(const string& name) {
    try {
        File dat(name, File::WRITE, File::CREATE | File::TRUNCATE);
        dat.setPos(1024 * 1024);
        dat.setEOF();
        dat.setPos(0);
        int64_t start = sizeof(start);
        dat.write(&start, sizeof(start));

    } catch (const FileException& e) {
        LogManager::getInstance()->message(str(F_("Error creating hash data file: %1%") % e.getError()));
    }
}


} // namespace dcpp
