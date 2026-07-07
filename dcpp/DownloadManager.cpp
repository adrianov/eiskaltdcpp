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

void DownloadManager::on(TimerManagerListener::Second, uint64_t aTick) noexcept {
    typedef vector<pair<string, UserPtr> > TargetList;
    TargetList dropTargets;

    {
        Lock l(cs);

        DownloadList tickList;
        // Tick each ongoing download
        for(auto i: downloads) {
            if(i->getPos() > 0) {
                tickList.push_back(i);
                i->tick();
            }
        }

        if(!tickList.empty())
            fire(DownloadManagerListener::Tick(), tickList);


        // Automatically remove or disconnect slow sources
        if((uint32_t)(aTick / 1000) % SETTING(AUTODROP_INTERVAL) == 0) {
            for(auto d: downloads) {
                uint64_t timeElapsed = aTick - d->getStart();
                uint64_t timeInactive = aTick - d->getUserConnection().getLastActivity();
                uint64_t bytesDownloaded = d->getPos();
                bool timeElapsedOk = timeElapsed >= (uint32_t)SETTING(AUTODROP_ELAPSED) * 1000;
                bool timeInactiveOk = timeInactive <= (uint32_t)SETTING(AUTODROP_INACTIVITY) * 1000;
                bool speedTooLow = timeElapsedOk && timeInactiveOk && bytesDownloaded > 0 ?
                            bytesDownloaded / timeElapsed * 1000 < (uint32_t)SETTING(AUTODROP_SPEED) : false;
                bool isUserList = d->getType() == Transfer::TYPE_FULL_LIST;
                bool onlineSourcesOk = isUserList ?
                            true : QueueManager::getInstance()->countOnlineSources(d->getPath()) >= SETTING(AUTODROP_MINSOURCES);
                bool filesizeOk = !isUserList && d->getSize() >= ((int64_t)SETTING(AUTODROP_FILESIZE)) * 1024;
                bool dropIt = (isUserList && BOOLSETTING(AUTODROP_FILELISTS)) ||
                        (filesizeOk && BOOLSETTING(AUTODROP_ALL));
                if(speedTooLow && onlineSourcesOk && dropIt) {
                    if(BOOLSETTING(AUTODROP_DISCONNECT) && isUserList) {
                        d->getUserConnection().disconnect();
                    } else {
                        dropTargets.emplace_back(d->getPath(), d->getUser());
                    }
                }
            }
        }
    }
    for(auto& i: dropTargets) {
        QueueManager::getInstance()->removeSource(i.first, i.second, QueueItem::Source::FLAG_SLOW_SOURCE);
    }
}

void DownloadManager::checkIdle(const UserPtr& user) {
    Lock l(cs);
    for(auto uc: idlers) {
        if(uc->getUser() == user) {
            uc->updated();
            return;
        }
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

    // TLS is required for ADC transfers only; NMDC transfers are usually unencrypted
    if(SETTING(REQUIRE_TLS) && !conn->isSet(UserConnection::FLAG_NMDC) && !conn->isSecure()) {
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

void DownloadManager::on(AdcCommand::SND, UserConnection* aSource, const AdcCommand& cmd) noexcept {
    if(aSource->getState() != UserConnection::STATE_SND) {
        dcdebug("DM::onFileLength Bad state, ignoring\n");
        return;
    }

    const string& type = cmd.getParam(0);
    int64_t start = Util::toInt64(cmd.getParam(2));
    int64_t bytes = Util::toInt64(cmd.getParam(3));

    if(type != Transfer::names[aSource->getDownload()->getType()]) {
        // Uhh??? We didn't ask for this...
        aSource->disconnect();
        return;
    }

    startData(aSource, start, bytes, cmd.hasFlag("ZL", 4));
}

void DownloadManager::startData(UserConnection* aSource, int64_t start, int64_t bytes, bool z) {
    Download* d = aSource->getDownload();
    dcassert(d != NULL);

    dcdebug("Preparing " I64_FMT ":" I64_FMT ", " I64_FMT ":" I64_FMT"\n",
            static_cast<long long int>(d->getStartPos()), static_cast<long long int>(start),
            static_cast<long long int>(d->getSize()), static_cast<long long int>(bytes));
    if(d->getSize() == -1) {
        if(bytes >= 0) {
            d->setSize(bytes);
        } else {
            failDownload(aSource, _("Invalid size"));
            return;
        }
    } else if(d->getSize() != bytes || d->getStartPos() != start) {
        // This is not what we requested...
        failDownload(aSource, _("Response does not match request"));
        return;
    }

    try {
        QueueManager::getInstance()->setFile(d);
    } catch(const FileException& e) {
        failDownload(aSource, str(F_("Could not open target file: %1%") % e.getError()));
        return;
    } catch(const Exception& e) {
        failDownload(aSource, e.getError());
        return;
    }

    if((d->getType() == Transfer::TYPE_FILE || d->getType() == Transfer::TYPE_FULL_LIST) && SETTING(BUFFER_SIZE) > 0 ) {
        d->setFile(new BufferedOutputStream<true>(d->getFile()));
    }

    if(d->getType() == Transfer::TYPE_FILE) {
        typedef MerkleCheckOutputStream<TigerTree, true> MerkleStream;

        d->setFile(new MerkleStream(d->getTigerTree(), d->getFile(), d->getStartPos()));
        d->setFlag(Download::FLAG_TTH_CHECK);
    }

    // Check that we don't get too many bytes
    d->setFile(new LimitedOutputStream<true>(d->getFile(), bytes));

    if(z) {
        d->setFlag(Download::FLAG_ZDOWNLOAD);
        d->setFile(new FilteredOutputStream<UnZFilter, true>(d->getFile()));
    }

    d->setStart(GET_TICK());
    d->tick();
    aSource->setState(UserConnection::STATE_RUNNING);

    fire(DownloadManagerListener::Starting(), d);

    if(d->getPos() == d->getSize()) {
        try {
            // Already finished? A zero-byte file list could cause this...
            endData(aSource);
        } catch(const Exception& e) {
            failDownload(aSource, e.getError());
        }
    } else {
        aSource->setDataMode();
    }
}

void DownloadManager::on(UserConnectionListener::Data, UserConnection* aSource, const uint8_t* aData, size_t aLen) noexcept {
    Download* d = aSource->getDownload();
    dcassert(d != NULL);

    try {
        d->addPos(d->getFile()->write(aData, aLen), aLen);
        d->tick();

        if(d->getFile()->eof()) {
            endData(aSource);
            aSource->setLineMode(0);
        }
    } catch(const Exception& e) {
        failDownload(aSource, e.getError());
    }
}

/** Download finished! */
void DownloadManager::endData(UserConnection* aSource) {
    dcassert(aSource->getState() == UserConnection::STATE_RUNNING);
    Download* d = aSource->getDownload();
    dcassert(d != NULL);

    if(d->getType() == Transfer::TYPE_TREE) {
        d->getFile()->flush();

        int64_t bl = 1024;
        while(bl * (int64_t)d->getTigerTree().getLeaves().size() < d->getTigerTree().getFileSize())
            bl *= 2;
        d->getTigerTree().setBlockSize(bl);
        d->getTigerTree().calcRoot();

        if(!(d->getTTH() == d->getTigerTree().getRoot())) {
            // This tree is for a different file, remove from queue...
            removeDownload(d);
            fire(DownloadManagerListener::Failed(), d, _("Full tree does not match TTH root"));

            QueueManager::getInstance()->removeSource(d->getPath(), aSource->getUser(), QueueItem::Source::FLAG_BAD_TREE, false);

            QueueManager::getInstance()->putDownload(d, false);

            checkDownloads(aSource);
            return;
        }
        d->setTreeValid(true);
    } else {
        // First, finish writing the file (flushing the buffers and closing the file...)
        try {
            d->getFile()->flush();
        } catch(const Exception& e) {
            d->resetPos();
            failDownload(aSource, e.getError());
            return;
        }

        aSource->setSpeed(d->getAverageSpeed());
        aSource->updateChunkSize(d->getTigerTree().getBlockSize(), d->getSize(), GET_TICK() - d->getStart());

        dcdebug("Download finished: %s, size " I64_FMT ", downloaded " I64_FMT "\n", d->getPath().c_str(),
                static_cast<long long int>(d->getSize()), static_cast<long long int>(d->getPos()));

        if(BOOLSETTING(LOG_DOWNLOADS) && (BOOLSETTING(LOG_FILELIST_TRANSFERS) || d->getType() == Transfer::TYPE_FILE)) {
            logDownload(aSource, d);
        }
    }

    removeDownload(d);
    fire(DownloadManagerListener::Complete(), d);

    QueueManager::getInstance()->putDownload(d, true);
    checkDownloads(aSource);
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
