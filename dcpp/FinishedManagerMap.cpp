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

#include "FinishedManager.h"

#include "FinishedItem.h"
#include "FinishedManagerListener.h"
#include "ClientManager.h"
#include "File.h"
#include "Segment.h"
#include "MerkleTree.h"
#include "format.h"

namespace dcpp {

void FinishedManager::remove(bool upload, const string& file) {
    {
        Lock l(cs);
        MapByFile& map = upload ? ULByFile : DLByFile;
        auto it = map.find(file);
        if(it != map.end())
            map.erase(it);
        else
            return;
        if(!upload) {
            for(auto i = dlByTth.begin(); i != dlByTth.end();) {
                if(i->second == file)
                    i = dlByTth.erase(i);
                else
                    ++i;
            }
        }
    }
    fire(FinishedManagerListener::RemovedFile(), upload, file);
}

void FinishedManager::remove(bool upload, const HintedUser& user) {
    {
        Lock l(cs);
        MapByUser& map = upload ? ULByUser : DLByUser;
        auto it = map.find(user);
        if(it != map.end())
            map.erase(it);
        else
            return;
    }
    fire(FinishedManagerListener::RemovedUser(), upload, user);
}

void FinishedManager::removeAll(bool upload) {
    if(upload)
        clearULs();
    else
        clearDLs();
    fire(FinishedManagerListener::RemovedAll(), upload);
}

void FinishedManager::clearDLs() {
    Lock l(cs);
    DLByFile.clear();
    DLByUser.clear();
    dlByTth.clear();
}

void FinishedManager::clearULs() {
    Lock l(cs);
    ULByFile.clear();
    ULByUser.clear();
}

void FinishedManager::getParams(const string & target, ParamMap& params) {
    Lock l(cs);

    auto it = DLByFile.find(target);
    if(it == DLByFile.end()) {
        return;
    }

    auto entry = it->second;
    if (!entry->getUsers().empty()) {
        StringList nicks, cids, ips, hubNames, hubUrls, temp;
        string ip;
        for(auto& i: entry->getUsers()) {

            nicks.push_back(Util::toString(ClientManager::getInstance()->getNicks(i)));
            cids.push_back(i.user->getCID().toBase32());

            ip.clear();
            if (i.user->isOnline()) {
                OnlineUser* u = ClientManager::getInstance()->findOnlineUser(i, false);
                if (u) {
                    ip = u->getIdentity().getIp();
                }
            }
            if (ip.empty()) {
                ip = _("Offline");
            }
            ips.push_back(ip);

            temp = ClientManager::getInstance()->getHubNames(i);
            if(temp.empty()) {
                temp.push_back(_("Offline"));
            }
            hubNames.push_back(Util::toString(temp));

            temp = ClientManager::getInstance()->getHubUrls(i);
            if(temp.empty()) {
                temp.push_back(_("Offline"));
            }
            hubUrls.push_back(Util::toString(temp));
        }

        params["userNI"] = Util::toString(nicks);
        params["userCID"] = Util::toString(cids);
        params["userI4"] = Util::toString(ips);
        params["hubNI"] = Util::toString(hubNames);
        params["hubURL"] = Util::toString(hubUrls);
    }

    params["fileSIsession"] = Util::toString(entry->getTransferred());
    params["fileSIsessionshort"] = Util::formatBytes(entry->getTransferred());
    params["fileSIactual"] = Util::toString(entry->getActual());
    params["fileSIactualshort"] = Util::formatBytes(entry->getActual());

    params["speed"] = str(F_("%1%/s") % Util::formatBytes(entry->getAverageSpeed()));
    params["time"] = Util::formatSeconds(entry->getMilliSeconds() / 1000);
}

string FinishedManager::getTarget(const string& aTTH){
    if(aTTH.empty()) return Util::emptyString;

    Lock l(cs);
    auto it = dlByTth.find(aTTH);
    if(it == dlByTth.end())
        return Util::emptyString;
    if(File::getSize(it->second) < 0) {
        dlByTth.erase(it);
        return Util::emptyString;
    }
    return it->second;
}

bool FinishedManager::handlePartialRequest(const TTHValue& tth, vector<uint16_t>& outPartialInfo)
{
    string target = getTarget(tth.toBase32());

    if(target.empty()) return false;

    int64_t fileSize = File::getSize(target);

    if(fileSize < PARTIAL_SHARE_MIN_SIZE)
        return false;

    uint16_t len = TigerTree::calcBlocks(fileSize,(int)100);
    outPartialInfo.push_back(0);
    outPartialInfo.push_back(len);

    return true;
}

} // namespace dcpp
