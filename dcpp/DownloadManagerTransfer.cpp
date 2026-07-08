/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "DownloadManager.h"

#include "Download.h"
#include "FilteredFile.h"
#include "LogManager.h"
#include "MerkleCheckOutputStream.h"
#include "QueueManager.h"
#include "Transfer.h"
#include "UserConnection.h"
#include "ZUtils.h"

namespace dcpp {

void DownloadManager::on(AdcCommand::SND, UserConnection* aSource, const AdcCommand& cmd) noexcept {
    if(aSource->getState() != UserConnection::STATE_SND) {
        dcdebug("DM::onFileLength Bad state, ignoring\n");
        return;
    }

    const string& type = cmd.getParam(0);
    int64_t start = Util::toInt64(cmd.getParam(2));
    int64_t bytes = Util::toInt64(cmd.getParam(3));

    if(type != Transfer::names[aSource->getDownload()->getType()]) {
        aSource->disconnect();
        return;
    }

    startData(aSource, start, bytes, cmd.hasFlag("ZL", 4));
}

void DownloadManager::startData(UserConnection* aSource, int64_t start, int64_t bytes, bool z) {
    Download* d = aSource->getDownload();
    dcassert(d != NULL);

    if(start == 0 && d->getType() == Transfer::TYPE_FILE)
        aSource->resetTransferSpeed();

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
        failDownload(aSource, _("Response does not match request"));
        return;
    }

    try {
        QueueManager::getInstance()->setFile(d);
    } catch(const FileException& e) {
        failDownload(aSource, Util::isNoSpaceMessage(e.getError()) ? e.getError() : str(F_("Could not open target file: %1%") % e.getError()));
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
        aSource->updateTransferSpeed(d->getStartPos() + d->getPos());

        if(d->getFile()->eof()) {
            endData(aSource);
            aSource->setLineMode(0);
        }
    } catch(const Exception& e) {
        failDownload(aSource, e.getError());
    }
}

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
            removeDownload(d);
            fire(DownloadManagerListener::Failed(), d, _("Full tree does not match TTH root"));

            QueueManager::getInstance()->removeSource(d->getPath(), aSource->getUser(), QueueItem::Source::FLAG_BAD_TREE, false);

            QueueManager::getInstance()->putDownload(d, false);

            checkDownloads(aSource);
            return;
        }
        d->setTreeValid(true);
    } else {
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

} // namespace dcpp
