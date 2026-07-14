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
#include "QueueItem.h"

#include "ClientManager.h"
#include "File.h"
#include "PeerConnectFilter.h"
#include "QueueManager.h"
#include "Util.h"

namespace dcpp {

namespace {
const string TEMP_EXTENSION = ".dctmp";

string getTempName(const string& aFileName, const TTHValue& aRoot) {
    string tmp(aFileName);
    tmp += "." + aRoot.toBase32();
    tmp += TEMP_EXTENSION;
    return tmp;
}
}

void QueueItem::getOnlineUsers(HintedUserList& l, const unordered_set<CID>& queuedDownloads) const {
    auto* qm = QueueManager::getInstance();
    for(auto& i: sources) {
        if(!i.getUser().user->isOnline())
            continue;
        OnlineUser* ou = ClientManager::getInstance()->findOnlineUser(i.getUser(), false);
        if(ou && !PeerConnectFilter::isViablePeer(*ou))
            continue;
        if(!qm->shouldConnectSource(this, i.getUser(), queuedDownloads))
            continue;
        l.push_back(i.getUser());
        if(isSet(FLAG_USER_LIST))
            return;
    }
}

void QueueItem::addSource(const HintedUser& aUser) {
    dcassert(!isSource(aUser.user));
    auto i = getBadSource(aUser);
    if(i != badSources.end()) {
        sources.push_back(*i);
        badSources.erase(i);
    } else {
        sources.emplace_back(aUser);
    }
}

void QueueItem::removeSource(const UserPtr& aUser, int reason) {
    auto i = getSource(aUser);
    dcassert(i != sources.end());
    i->setFlag(reason);
    badSources.push_back(*i);
    sources.erase(i);
}

const string& QueueItem::getTempTarget() {
    if(!isSet(QueueItem::FLAG_USER_LIST) && tempTarget.empty()) {
        if(!SETTING(TEMP_DOWNLOAD_DIRECTORY).empty() && (File::getSize(getTarget()) == -1)) {
            string tmp;
#ifdef _WIN32
            dcpp::StringMap sm;
            if(target.length() >= 3 && target[1] == ':' && target[2] == '\\')
                sm["targetdrive"] = target.substr(0, 3);
            else
                sm["targetdrive"] = Util::getPath(Util::PATH_USER_LOCAL).substr(0, 3);
            if (SETTING(NO_USE_TEMP_DIR))
                tmp = Util::formatParams(target, sm, false) + getTempName("", getTTH());
            else
                tmp = Util::formatParams(SETTING(TEMP_DOWNLOAD_DIRECTORY), sm, false) + getTempName(getTargetFileName(), getTTH());
#else //_WIN32
            if (SETTING(NO_USE_TEMP_DIR))
                tmp = target + getTempName("", getTTH());
            else
                tmp = SETTING(TEMP_DOWNLOAD_DIRECTORY) + getTempName(getTargetFileName(), getTTH());
#endif //_WIN32
            int len = Util::getFileName(tmp).size()+1;
            if (len >= 255){
                string tmp1 = tmp.erase(tmp.find(".dctmp")-33,tmp.find(".dctmp")+5) + ".dctmp";
                tmp = tmp1;
            }
            len = Util::getFileName(tmp).size()+1;
            if (len >= 255) {
                char tmp3[206];
                string tmp2 = tmp.erase(tmp.find(".dctmp")-7,tmp.find(".dctmp")+5);
                memcpy (tmp3, Util::getFileName(tmp2).c_str(), 206);
                tmp = Util::getFilePath(tmp) + string(tmp3) + "~." + getTTH().toBase32() + ".dctmp";
            }
            setTempTarget(tmp);
        }
    }
    return tempTarget;
}

int64_t QueueItem::getDownloadedBytes() const {
    int64_t total = 0;
    for(auto& i: done) {
        total += i.getSize();
    }
    return total;
}

void QueueItem::addSegment(const Segment& segment) {
    done.insert(segment);

    // Consolidate segments
    if(done.size() == 1)
        return;

    for(auto i = ++done.begin() ; i != done.end(); ) {
        auto prev = i;
        --prev;
        if(prev->getEnd() >= i->getStart()) {
            Segment big(prev->getStart(), i->getEnd() - prev->getStart());
            done.erase(prev);
            done.erase(i++);
            done.insert(big);
        } else {
            ++i;
        }
    }
}

string QueueItem::getListName() const {
    dcassert(isSet(QueueItem::FLAG_USER_LIST));
    if(isSet(QueueItem::FLAG_XML_BZLIST)) {
        return getTarget() + ".xml.bz2";
    } else {
        return getTarget() + ".xml";
    }
}

// Partial
bool QueueItem::isNeededPart(const PartsInfo& partsInfo, int64_t blockSize) const {
    auto i  = done.begin();
    for(auto j = partsInfo.begin(); j != partsInfo.end(); j+=2) {
        while(i != done.end() && (*i).getEnd() <= (*j) * blockSize)
            ++i;

        if(i == done.end() || !((*i).getStart() <= (*j) * blockSize && (*i).getEnd() >= (*(j+1)) * blockSize))
            return true;
    }

    return false;
}

void QueueItem::getPartialInfo(PartsInfo& partialInfo, int64_t blockSize) const {
    size_t maxSize = min(done.size() * 2, (size_t)510);
    partialInfo.reserve(maxSize);

    for(auto& i : done) {
        if(partialInfo.size() >= maxSize)
            break;

        uint16_t s = (uint16_t)(i.getStart() / blockSize);
        uint16_t e = (uint16_t)((i.getEnd() - 1) / blockSize + 1);

        partialInfo.push_back(s);
        partialInfo.push_back(e);
    }
}

}
