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
#include "share/ShareFileList.h"

#include "BZUtils.h"
#include "ClientManager.h"
#include "File.h"
#include "FilteredFile.h"
#include "LogManager.h"
#include "ShareManager.h"
#include "SimpleXML.h"
#include "SimpleXMLReader.h"
#include "Streams.h"
#include "Util.h"
#include "UserConnection.h"
#include "format.h"
#include "version.h"

namespace dcpp {

#define LITERAL(n) n, sizeof(n)-1

static const string SDIRECTORY = "Directory";
static const string SFILE = "File";
static const string SNAME = "Name";
static const string SSIZE = "Size";
static const string STTH = "TTH";

struct ShareLoader : public SimpleXMLReader::CallBack {
    ShareLoader(ShareManager::DirList& aDirs) : dirs(aDirs), cur(0), depth(0) { }
    virtual void startTag(const string& name, StringPairList& attribs, bool simple) {
        if(name == SDIRECTORY) {
            const string& name = getAttrib(attribs, SNAME, 0);
            if(!name.empty()) {
                if(depth == 0) {
                    for(auto& i : dirs) {
                        if(Util::stricmp(i->getName(), name) == 0) {
                            cur = i;
                            break;
                        }
                    }
                } else if(cur) {
                    cur = ShareDirectory::create(name, cur);
                    cur->getParent()->directories[cur->getName()] = cur;
                }
            }

            if(simple) {
                if(cur) {
                    cur = cur->getParent();
                }
            } else {
                depth++;
            }
        } else if(cur && name == SFILE) {
            const string& fname = getAttrib(attribs, SNAME, 0);
            const string& size = getAttrib(attribs, SSIZE, 1);
            const string& root = getAttrib(attribs, STTH, 2);
            if(fname.empty() || size.empty() || (root.size() != 39)) {
                dcdebug("Invalid file found: %s\n", fname.c_str());
                return;
            }
            cur->files.insert(ShareDirectory::File(fname, Util::toInt64(size), cur, TTHValue(root)));
        }
    }
    virtual void endTag(const string& name) {
        if(name == SDIRECTORY) {
            depth--;
            if(cur) {
                cur = cur->getParent();
            }
        }
    }

private:
    ShareManager::DirList& dirs;
    ShareDirectory::Ptr cur;
    size_t depth;
};

ShareFileList::ShareFileList(ShareManager& share) :
    share(share), xmlListLen(0), bzXmlListLen(0),
    xmlDirty(true), forceXmlRefresh(false), listN(0), lastXmlUpdate(0)
{
}

ShareFileList::~ShareFileList() {
    if(bzXmlRef.get()) {
        bzXmlRef.reset();
        File::deleteFile(bzXmlFile);
    }
}

string ShareFileList::diskPath() {
    return Util::getPath(Util::PATH_USER_CONFIG) + "files.xml.bz2";
}

const string& ShareFileList::ensure() {
    generate();
    if(File::getSize(bzXmlFile) != -1)
        return bzXmlFile;
    throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
}

bool ShareFileList::loadCache() noexcept {
    try {
        ShareLoader loader(share.directories);
        SimpleXMLReader xml(&loader);

        const string xmlList = diskPath();
        dcpp::File ff(xmlList, dcpp::File::READ, dcpp::File::OPEN);
        FilteredInputStream<UnBZFilter, false> f(&ff);

        xml.parse(f);

        for(auto& d : share.directories) {
            share.updateIndices(*d);
        }

        bzXmlFile = xmlList;
        bzXmlListLen = File::getSize(xmlList);
        return true;
    } catch(const Exception& e) {
        dcdebug("%s\n", e.getError().c_str());
    }
    return false;
}

void ShareFileList::generate() {
    Lock l(share.cs);
    if(File::getSize(bzXmlFile) == -1)
        bzXmlFile.clear();
    if(forceXmlRefresh || bzXmlFile.empty() || (xmlDirty && (lastXmlUpdate + 15 * 60 * 1000 < GET_TICK() || lastXmlUpdate < share.lastFullUpdate))) {
        listN++;

        try {
            string tmp2;
            string indent;

            string newXmlName = Util::getPath(Util::PATH_USER_CONFIG) + "files" + Util::toString(listN) + ".xml.bz2";
            {
                File f(newXmlName, File::WRITE, File::TRUNCATE | File::CREATE);
                CalcOutputStream<TTFilter<1024*1024*1024>, false> bzTree(&f);
                FilteredOutputStream<BZFilter, false> bzipper(&bzTree);
                CountOutputStream<false> count(&bzipper);
                CalcOutputStream<TTFilter<1024*1024*1024>, false> newXmlFile(&count);

                newXmlFile.write(SimpleXML::utf8Header);
                newXmlFile.write("<FileListing Version=\"1\" CID=\"" + ClientManager::getInstance()->getMe()->getCID().toBase32() + "\" Base=\"/\" Generator=\"" APPNAME " " VERSIONSTRING "\">\r\n");
                for(auto& i: share.directories) {
                    i->toXml(newXmlFile, indent, tmp2, true);
                }
                newXmlFile.write("</FileListing>");
                newXmlFile.flush();

                xmlListLen = count.getCount();

                newXmlFile.getFilter().getTree().finalize();
                bzTree.getFilter().getTree().finalize();

                xmlRoot = newXmlFile.getFilter().getTree().getRoot();
                bzXmlRoot = bzTree.getFilter().getTree().getRoot();
            }
            const string XmlListFileName = diskPath();
            if(bzXmlRef.get()) {
                bzXmlRef.reset();
                try {
                    File::renameFile(XmlListFileName, XmlListFileName + ".bak");
                } catch(const FileException&) { }
            }

            try {
                File::renameFile(newXmlName, XmlListFileName);
                newXmlName = XmlListFileName;
            } catch(const FileException&) {
            }
            try {
                File::copyFile(XmlListFileName, XmlListFileName + ".bak");
            } catch(const FileException&) { }
            bzXmlRef = unique_ptr<File>(new File(newXmlName, File::READ, File::OPEN));
            bzXmlFile = newXmlName;
            bzXmlListLen = File::getSize(newXmlName);
            LogManager::getInstance()->message(str(F_("File list %1% generated") % Util::addBrackets(bzXmlFile)));
            xmlDirty = false;
            forceXmlRefresh = false;
            lastXmlUpdate = GET_TICK();
        } catch(const Exception&) {
        }
    }
}

MemoryInputStream* ShareFileList::generatePartial(const string& dir, bool recurse) const {
    if(dir[0] != '/' || dir[dir.size()-1] != '/')
        return 0;

    string xml = SimpleXML::utf8Header;
    string tmp;
    xml += "<FileListing Version=\"1\" CID=\"" + ClientManager::getInstance()->getMe()->getCID().toBase32() + "\" Base=\"" + SimpleXML::escape(dir, tmp, false) + "\" Generator=\"" APPNAME " " VERSIONSTRING "\">\r\n";
    StringRefOutputStream sos(xml);
    string indent = "\t";

    Lock l(share.cs);
    if(dir == "/") {
        for(auto& i: share.directories) {
            tmp.clear();
            i->toXml(sos, indent, tmp, recurse);
        }
    } else {
        string::size_type i = 1, j = 1;

        ShareDirectory::Ptr root;

        bool first = true;
        while( (i = dir.find('/', j)) != string::npos) {
            if(i == j) {
                j++;
                continue;
            }

            if(first) {
                first = false;
                auto it = share.getByVirtual(dir.substr(j, i-j));

                if(it == share.directories.end())
                    return 0;
                root = *it;

            } else {
                auto it2 = root->directories.find(dir.substr(j, i-j));
                if(it2 == root->directories.end()) {
                    return 0;
                }
                root = it2->second;
            }
            j = i + 1;
        }

        if(!root)
            return 0;

        for(auto& it2: root->directories) {
            it2.second->toXml(sos, indent, tmp, recurse);
        }
        root->filesToXml(sos, indent, tmp);
    }

    xml += "</FileListing>";
    return new MemoryInputStream(xml);
}

void ShareDirectory::toXml(OutputStream& xmlFile, string& indent, string& tmp2, bool fullList) const {
    xmlFile.write(indent);
    xmlFile.write(LITERAL("<Directory Name=\""));
    xmlFile.write(SimpleXML::escape(name, tmp2, true));

    if(fullList) {
        xmlFile.write(LITERAL("\">\r\n"));

        indent += '\t';
        for(auto& i: directories) {
            i.second->toXml(xmlFile, indent, tmp2, fullList);
        }

        filesToXml(xmlFile, indent, tmp2);

        indent.erase(indent.length()-1);
        xmlFile.write(indent);
        xmlFile.write(LITERAL("</Directory>\r\n"));
    } else {
        if(directories.empty() && files.empty()) {
            xmlFile.write(LITERAL("\" />\r\n"));
        } else {
            xmlFile.write(LITERAL("\" Incomplete=\"1\" />\r\n"));
        }
    }
}

void ShareDirectory::filesToXml(OutputStream& xmlFile, string& indent, string& tmp2) const {
    for(auto& f: files) {
        xmlFile.write(indent);
        xmlFile.write(LITERAL("<File Name=\""));
        xmlFile.write(SimpleXML::escape(f.getName(), tmp2, true));
        xmlFile.write(LITERAL("\" Size=\""));
        xmlFile.write(Util::toString(f.getSize()));
        xmlFile.write(LITERAL("\" TTH=\""));
        tmp2.clear();
        xmlFile.write(f.getTTH().toBase32(tmp2));
        // Flylink-compatible: TS gates media attrs in DirectoryListingLoader.
        xmlFile.write(LITERAL("\" TS=\""));
        xmlFile.write(Util::toString(f.getTS()));
        if (f.mediaInfo.bitrate) {
            xmlFile.write(LITERAL("\" BR=\""));
            xmlFile.write(Util::toString(f.mediaInfo.bitrate));
        }
        if (!f.mediaInfo.resolution.empty()) {
            xmlFile.write(LITERAL("\" WH=\""));
            tmp2 = f.mediaInfo.resolution;
            xmlFile.write(SimpleXML::escape(tmp2, true));
        }
        if (!f.mediaInfo.audio_info.empty()) {
            xmlFile.write(LITERAL("\" MA=\""));
            tmp2 = f.mediaInfo.audio_info;
            xmlFile.write(SimpleXML::escape(tmp2, true));
        }
        if (!f.mediaInfo.video_info.empty()) {
            xmlFile.write(LITERAL("\" MV=\""));
            tmp2 = f.mediaInfo.video_info;
            xmlFile.write(SimpleXML::escape(tmp2, true));
        }
        xmlFile.write(LITERAL("\"/>\r\n"));
    }
}

} // namespace dcpp
