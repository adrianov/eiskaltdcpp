/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2018 Boris Pek <tehnick-8@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "QueueManager.h"

#include "ClientManager.h"
#include "Download.h"
#include "FilteredFile.h"
#include "FinishedItem.h"
#include "FinishedManager.h"
#include "LogManager.h"
#include "SFVReader.h"
#include "SettingsManager.h"
#include "ZUtils.h"

namespace dcpp {

bool QueueManager::checkSfv(QueueItem* qi, Download* d) {
    SFVReader sfv(qi->getTarget());

    if(sfv.hasCRC()) {
        bool crcMatch = false;
        try {
            crcMatch = (calcCrc32(qi->getTempTarget()) == sfv.getCRC());
        } catch(const FileException& ) {
            // Couldn't read the file to get the CRC(!!!)
        }

        if(!crcMatch) {
            /// @todo There is a slight chance that something happens with a file while it's being saved to disk
            /// maybe calculate tth along with crc and if tth is ok and crc is not flag the file as bad at once
            /// if tth mismatches (possible disk error) then repair / redownload the file

            File::deleteFile(qi->getTempTarget());
            qi->resetDownloaded();
            dcdebug("QueueManager: CRC32 mismatch for %s\n", qi->getTarget().c_str());
            LogManager::getInstance()->message(_("CRC32 inconsistency (SFV-Check)") + string(" ") + Util::addBrackets(qi->getTarget()));

            setPriority(qi->getTarget(), QueueItem::PAUSED);

            QueueItem::SourceList sources = qi->getSources();
            for(auto& i: sources) {
                removeSource(qi->getTarget(), i.getUser(), QueueItem::Source::FLAG_CRC_FAILED, false);
            }

            fire(QueueManagerListener::CRCFailed(), d, _("CRC32 inconsistency (SFV-Check)"));
        } else {
            dcdebug("QueueManager: CRC32 match for %s\n", qi->getTarget().c_str());
            fire(QueueManagerListener::CRCChecked(), d);
        }
        return true;
    }
    return false;
}

uint32_t QueueManager::calcCrc32(const string& file) {
    File ff(file, File::READ, File::OPEN);
    CalcInputStream<CRC32Filter, false> f(&ff);

    const size_t BUF_SIZE = 1024*1024;
    std::unique_ptr<uint8_t[]> b(new uint8_t[BUF_SIZE]);
    size_t n = BUF_SIZE;
    while(f.read(&b[0], n) > 0)
        ;       // Keep on looping...

    return f.getFilter().getValue();
}

void QueueManager::logFinishedDownload(QueueItem* qi, Download*, bool crcChecked)
{
    StringMap params;
    params["target"] = qi->getTarget();
    params["fileSI"] = Util::toString(qi->getSize());
    params["fileSIshort"] = Util::formatBytes(qi->getSize());
    params["fileTR"] = qi->getTTH().toBase32();
    params["sfv"] = Util::toString(crcChecked ? 1 : 0);

    {
        auto lock = FinishedManager::getInstance()->lock();
        const FinishedManager::MapByFile& map = FinishedManager::getInstance()->getMapByFile(false);
        FinishedManager::MapByFile::const_iterator it = map.find(qi->getTarget());
        if(it != map.end()) {
            FinishedFileItemPtr entry = it->second;
            if (!entry->getUsers().empty()) {
                StringList nicks, cids, ips, hubNames, hubUrls, temp;
                string ip;
                for(HintedUserList::const_iterator i = entry->getUsers().begin(), iend = entry->getUsers().end(); i != iend; ++i) {

                    nicks.push_back(Util::toString(ClientManager::getInstance()->getNicks(*i)));
                    cids.push_back(i->user->getCID().toBase32());

                    ip.clear();
                    if (i->user->isOnline()) {
                        OnlineUser* u = ClientManager::getInstance()->findOnlineUser(*i, false);
                        if (u) {
                            ip = u->getIdentity().getIp();
                        }
                    }
                    if (ip.empty()) {
                        ip = _("Offline");
                    }
                    ips.push_back(ip);

                    temp = ClientManager::getInstance()->getHubNames(*i);
                    if(temp.empty()) {
                        temp.push_back(_("Offline"));
                    }
                    hubNames.push_back(Util::toString(temp));

                    temp = ClientManager::getInstance()->getHubs(*i);
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
    }

    LOG(LogManager::FINISHED_DOWNLOAD, params);
}

} // namespace dcpp
