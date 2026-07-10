/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
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

#include "DownloadManager.h"

#include "QueueManager.h"
#include "ClientManager.h"
#include "Download.h"
#include "LogManager.h"
#include "User.h"
#include "File.h"
#include "FilteredFile.h"
#include "MerkleCheckOutputStream.h"
#include "UserConnection.h"
#include "ZUtils.h"
#include "extra/ipfilter.h"
#include <limits>
#include <cmath>

// some strange mac definition
#ifdef ff
#undef ff
#endif

namespace dcpp {

static const string DOWNLOAD_AREA = "Downloads";

DownloadManager::DownloadManager() {
    TimerManager::getInstance()->addListener(this);
}

DownloadManager::~DownloadManager() {
    TimerManager::getInstance()->removeListener(this);
    while(true) {
        {
            Lock l(cs);
            if(downloads.empty())
                break;
        }
        Thread::sleep(100);
    }
}

void DownloadManager::addConnection(UserConnectionPtr conn) {

    if(!conn->isSet(UserConnection::FLAG_SUPPORTS_TTHF) || !conn->isSet(UserConnection::FLAG_SUPPORTS_ADCGET)) {
        // Can't download from these...
        LogManager::getInstance()->message(str(F_("Download rejected from %1%: client does not support ADC/TTH") % ClientManager::getInstance()->getNickOrCid(conn->getHintedUser())));
        conn->getUser()->setFlag(User::OLD_CLIENT);
        QueueManager::getInstance()->removeSource(conn->getUser(), QueueItem::Source::FLAG_NO_TTHF);
        conn->disconnect();
        return;
    }

    if (BOOLSETTING(IPFILTER) && !IPFilter::getInstance()->OK(conn->getRemoteIp(),eDIRECTION_IN)) {
        conn->error("Your IP is Blocked!");
        LogManager::getInstance()->message(_("IPFilter: Blocked outgoing connection to ") + conn->getRemoteIp());
        QueueManager::getInstance()->removeSource(conn->getUser(), QueueItem::Source::FLAG_REMOVED);
        conn->disconnect();
        return;
    }

    if(SETTING(REQUIRE_TLS) && !conn->isSecure()) {
        LogManager::getInstance()->message(str(F_("Download rejected from %1%: TLS required but connection is not encrypted") % ClientManager::getInstance()->getNickOrCid(conn->getHintedUser())));
        conn->error("Secure connection required!");
        QueueManager::getInstance()->removeSource(conn->getUser(), QueueItem::Source::FLAG_UNENCRYPTED);
        return;
    }

    conn->addListener(this);
    checkDownloads(conn);
}

bool DownloadManager::startDownload(QueueItem::Priority prio) {
    size_t downloadCount = getDownloadCount();

    bool full = (SETTING(DOWNLOAD_SLOTS) != 0) && (downloadCount >= (size_t)SETTING(DOWNLOAD_SLOTS));
    full = full || ((SETTING(MAX_DOWNLOAD_SPEED) != 0) && (getRunningAverage() >= (SETTING(MAX_DOWNLOAD_SPEED)*1024)));

    if(full) {
        bool extraFull = (SETTING(DOWNLOAD_SLOTS) != 0) && (getDownloadCount() >= (size_t)(SETTING(DOWNLOAD_SLOTS)+3));
        if(extraFull) {
            return false;
        }
        return prio == QueueItem::HIGHEST;
    }

    if(downloadCount > 0) {
        return prio != QueueItem::LOWEST;
    }

    return true;
}

void DownloadManager::checkDownloads(UserConnection* aConn) {
    QueueItem::Priority prio = QueueManager::getInstance()->hasDownload(aConn->getUser());
    if(!startDownload(prio)) {
        removeConnection(aConn);
        return;
    }

    Download* d = QueueManager::getInstance()->getDownload(*aConn, aConn->isSet(UserConnection::FLAG_SUPPORTS_TTHL));

    if(!d) {
        Lock l(cs);
        aConn->setState(UserConnection::STATE_IDLE);
        idlers.push_back(aConn);
        return;
    }

    aConn->setState(UserConnection::STATE_SND);

    if(aConn->isSet(UserConnection::FLAG_SUPPORTS_XML_BZLIST) && d->getType() == Transfer::TYPE_FULL_LIST) {
        d->setFlag(Download::FLAG_XML_BZ_LIST);
    }

    {
        Lock l(cs);
        downloads.push_back(d);
    }
    fire(DownloadManagerListener::Requesting(), d);

    dcdebug("Requesting " I64_FMT "/" I64_FMT "\n", static_cast<long long int>(d->getStartPos()), static_cast<long long int>(d->getSize()));

    aConn->send(d->getCommand(aConn->isSet(UserConnection::FLAG_SUPPORTS_ZLIB_GET)));
}

int64_t DownloadManager::getRunningAverage() {
    Lock l(cs);
    int64_t avg = 0;
    for(auto d: downloads) {
        avg += d->getAverageSpeed();
    }
    return avg;
}

void DownloadManager::logDownload(UserConnection* aSource, Download* d) {
    StringMap params;
    d->getParams(*aSource, params);
    LOG(LogManager::DOWNLOAD, params);
}

} // namespace dcpp
