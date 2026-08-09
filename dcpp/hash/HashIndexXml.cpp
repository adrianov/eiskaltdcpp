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
#include "hash/HashIndexXml.h"

#include "File.h"
#include "LogManager.h"
#include "SimpleXML.h"
#include "Streams.h"
#include "Util.h"
#include "format.h"
#include "hash/HashDataFile.h"
#include "hash/HashStore.h"

namespace dcpp {

namespace {

#define HASH_FILE_VERSION_STRING "2"
const int HASH_FILE_VERSION = 2;

const string sHashStore = "HashStore";
const string sversion = "version";
const string sVersion = "Version";
const string sTrees = "Trees";
const string sFiles = "Files";
const string sFile = "File";
const string sName = "Name";
const string sSize = "Size";
const string sHash = "Hash";
const string sType = "Type";
const string sTTH = "TTH";
const string sIndex = "Index";
const string sBlockSize = "BlockSize";
const string sTimeStamp = "TimeStamp";
const string sRoot = "Root";

} // namespace

class HashIndexXmlLoader : public SimpleXMLReader::CallBack {
public:
    explicit HashIndexXmlLoader(HashStore& s) :
        store(s), timeStamp(0), version(HASH_FILE_VERSION),
        inTrees(false), inFiles(false), inHashStore(false) {}

    void startTag(const string& name, StringPairList& attribs, bool simple) override;

private:
    HashStore& store;
    string file;
    uint32_t timeStamp;
    int version;
    bool inTrees;
    bool inFiles;
    bool inHashStore;
};

void HashIndexXmlLoader::startTag(const string& name, StringPairList& attribs, bool simple) {
    if (!inHashStore && name == sHashStore) {
        version = Util::toInt(getAttrib(attribs, sVersion, 0));
        if (version == 0)
            version = Util::toInt(getAttrib(attribs, sversion, 0));
        inHashStore = !simple;
    } else if (inHashStore && version == 2) {
        if (inTrees && name == sHash) {
            const string& type = getAttrib(attribs, sType, 0);
            int64_t index = Util::toInt64(getAttrib(attribs, sIndex, 1));
            int64_t blockSize = Util::toInt64(getAttrib(attribs, sBlockSize, 2));
            int64_t size = Util::toInt64(getAttrib(attribs, sSize, 3));
            const string& root = getAttrib(attribs, sRoot, 4);
            if (!root.empty() && type == sTTH && (index >= 8 || index == HashDataFile::SMALL_TREE) && blockSize >= 1024)
                store.treeIndex[TTHValue(root)] = HashStore::TreeInfo(size, index, blockSize);
        } else if (inFiles && name == sFile) {
            file = getAttrib(attribs, sName, 0);
            timeStamp = Util::toUInt32(getAttrib(attribs, sTimeStamp, 1));
            const string& root = getAttrib(attribs, sRoot, 2);
            if (!file.empty() && timeStamp > 0 && !root.empty()) {
                store.fileIndex[Util::getFilePath(file)].emplace_back(
                    Util::getFileName(file), TTHValue(root), timeStamp, false);
            }
        } else if (name == sTrees) {
            inTrees = !simple;
        } else if (name == sFiles) {
            inFiles = !simple;
        }
    }
}

string HashIndexXml::path() {
    return Util::getPath(Util::PATH_USER_CONFIG) + "HashIndex.xml";
}

void HashIndexXml::load(HashStore& store) {
    try {
        Util::migrate(path());
        HashIndexXmlLoader loader(store);
        File f(path(), File::READ, File::OPEN);
        SimpleXMLReader(&loader).parse(f);
    } catch (const Exception&) {
    }
}

void HashIndexXml::save(HashStore& store) {
    if (!store.dirty)
        return;
    try {
        File ff(path() + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
        BufferedOutputStream<false> f(&ff);
        string tmp;
        string b32tmp;

        f.write(SimpleXML::utf8Header);
        f.write(LIT("<HashStore Version=\"" HASH_FILE_VERSION_STRING "\">\r\n"));
        f.write(LIT("\t<Trees>\r\n"));
        for (auto& i : store.treeIndex) {
            const auto& ti = i.second;
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
        for (auto& i : store.fileIndex) {
            const string& dir = i.first;
            for (auto& fi : i.second) {
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
        File::deleteFile(path());
        File::renameFile(path() + ".tmp", path());
        store.dirty = false;
    } catch (const FileException& e) {
        LogManager::getInstance()->message(str(F_("Error saving hash data: %1%") % e.getError()));
    }
}

} // namespace dcpp
