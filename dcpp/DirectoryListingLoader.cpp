/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "stdinc.h"
#include "DirectoryListingLoader.h"

#include "SimpleXML.h"
#include "StringTokenizer.h"

namespace dcpp {

ListLoader::ListLoader(DirectoryListing::Directory* root, bool aUpdating) :
    cur(root),
    base("/"),
    inListing(false),
    updating(aUpdating),
    m_is_mediainfo_list(false),
    m_is_first_check_mediainfo_list(false)
{
}

static const string sFileListing = "FileListing";
static const string sBase = "Base";
static const string sDirectory = "Directory";
static const string sIncomplete = "Incomplete";
static const string sFile = "File";
static const string sName = "Name";
static const string sSize = "Size";
static const string sTTH = "TTH";
static const string sBR = "BR";
static const string sWH = "WH";
static const string sMVideo = "MV";
static const string sMAudio = "MA";
static const string sTS = "TS";
static const string sHIT = "HIT";

void ListLoader::startTag(const string& name, StringPairList& attribs, bool simple) {
    if(inListing) {
        if(name == sFile) {
            const string& n = getAttrib(attribs, sName, 0);
            if(n.empty())
                return;
            const string& s = getAttrib(attribs, sSize, 1);
            if(s.empty())
                return;
            auto size = Util::toInt64(s);
            const string& h = getAttrib(attribs, sTTH, 2);
            if(h.empty())
                return;
            TTHValue tth(h);

            if(updating) {
                for(auto& i : cur->files) {
                    auto& file = *i;
                    if(file.getTTH() == tth || file.getName() == n) {
                        file.setName(n);
                        file.setSize(size);
                        file.setTTH(tth);
                        return;
                    }
                }
            }

            DirectoryListing::File* f = new DirectoryListing::File(cur, n, size, tth);

            string l_ts;
            if (!m_is_first_check_mediainfo_list){
                m_is_first_check_mediainfo_list = true;
                l_ts = getAttrib(attribs, sTS, 3);
                m_is_mediainfo_list = !l_ts.empty();
            }
            else if (m_is_mediainfo_list) {
                l_ts = getAttrib(attribs, sTS, 3);
            }

            if (!l_ts.empty()){
                f->setTS(atol(l_ts.c_str()));
                f->setHit(atol(getAttrib(attribs, sHIT, 3).c_str()));
                f->mediaInfo.video_info = getAttrib(attribs, sMVideo, 3);
                f->mediaInfo.audio_info = getAttrib(attribs, sMAudio, 3);
                f->mediaInfo.resolution = getAttrib(attribs, sWH, 3);
                f->mediaInfo.bitrate    = atoi(getAttrib(attribs, sBR, 4).c_str());
            }

            cur->files.insert(f);
        } else if(name == sDirectory) {
            const string& n = getAttrib(attribs, sName, 0);
            if(n.empty()) {
                throw SimpleXMLException(_("Directory missing name attribute"));
            }
            bool incomp = getAttrib(attribs, sIncomplete, 1) == "1";
            DirectoryListing::Directory* d = nullptr;
            if(updating) {
                for(auto& i : cur->directories) {
                    if(i->getName() == n) {
                        d = i;
                        if(!d->getComplete())
                            d->setComplete(!incomp);
                        break;
                    }
                }
            }
            if(!d) {
                d = new DirectoryListing::Directory(cur, n, false, !incomp);
                cur->directories.insert(d);
            }
            cur = d;

            if(simple)
                endTag(name);
        }
    } else if(name == sFileListing) {
        const string& b = getAttrib(attribs, sBase, 2);
        if(!b.empty() && b[0] == '/' && b[b.size()-1] == '/') {
            base = b;
        }
        StringList sl = StringTokenizer<string>(base.substr(1), '/').getTokens();
        for(auto& i: sl) {
            DirectoryListing::Directory* d = nullptr;
            for(auto j: cur->directories) {
                if(j->getName() == i) {
                    d = j;
                    break;
                }
            }
            if(!d) {
                d = new DirectoryListing::Directory(cur, i, false, false);
                cur->directories.insert(d);
            }
            cur = d;
        }
        cur->setComplete(true);
        inListing = true;

        if(simple)
            endTag(name);
    }
}

void ListLoader::endTag(const string& name) {
    if(!inListing)
        return;
    if(name == sDirectory) {
        cur = cur->getParent();
    } else if(name == sFileListing) {
        inListing = false;
    }
}

} // namespace dcpp
