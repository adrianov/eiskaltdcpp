/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "DownloadManager.h"

#include "Download.h"
#include "QueueManager.h"
#include "Transfer.h"
#include "UserConnection.h"
#include "format.h"

namespace dcpp {

namespace {

bool isFileNotAvailableError(const string& msg) {
    if(Util::stricmp(msg.c_str(), UserConnection::FILE_NOT_AVAILABLE) == 0)
        return true;
    static const char suffix[] = " no more exists";
    const size_t suffixLen = sizeof(suffix) - 1;
    return msg.length() >= suffixLen && msg.compare(msg.length() - suffixLen, suffixLen, suffix) == 0;
}

} // namespace

void DownloadManager::onFailed(UserConnection* aSource, const string& aError) {
    {
        Lock l(cs);
        idlers.erase(remove(idlers.begin(), idlers.end(), aSource), idlers.end());
    }
    failDownload(aSource, aError);
}

void DownloadManager::failDownload(UserConnection* aSource, const string& reason) {

    Download* d = aSource->getDownload();

    if(d && isFileNotAvailableError(reason)) {
        fileNotAvailable(aSource);
        return;
    }

    if(d) {
        if(Util::isNoSpaceMessage(reason) && d->getType() == Transfer::TYPE_FILE)
            QueueManager::getInstance()->handleDiskFull(d->getPath());
        removeDownload(d);
        fire(DownloadManagerListener::Failed(), d, reason);

        QueueManager::getInstance()->putDownload(d, false);
    }

    removeConnection(aSource);
}

void DownloadManager::removeConnection(UserConnectionPtr aConn) {
    dcassert(aConn->getDownload() == NULL);
    aConn->removeListener(this);
    aConn->disconnect();
}

void DownloadManager::removeDownload(Download* d) {
    if(d->getFile()) {
        if(d->getActual() > 0) {
            try {
                d->getFile()->flush();
            } catch(const Exception&) {
            }
        }
    }

    {
        Lock l(cs);
        dcassert(find(downloads.begin(), downloads.end(), d) != downloads.end());

        downloads.erase(remove(downloads.begin(), downloads.end(), d), downloads.end());
    }
}

void DownloadManager::on(UserConnectionListener::FileNotAvailable, UserConnection* aSource) noexcept {
    fileNotAvailable(aSource);
}

void DownloadManager::on(AdcCommand::STA, UserConnection* aSource, const AdcCommand& cmd) noexcept {
    if(cmd.getParameters().size() < 2) {
        aSource->disconnect();
        return;
    }

    const string& err = cmd.getParameters()[0];
    if(err.length() != 3) {
        aSource->disconnect();
        return;
    }

    switch(Util::toInt(err.substr(0, 1))) {
    case AdcCommand::SEV_FATAL:
        aSource->disconnect();
        return;
    case AdcCommand::SEV_RECOVERABLE:
        switch(Util::toInt(err.substr(1))) {
        case AdcCommand::ERROR_FILE_NOT_AVAILABLE:
            fileNotAvailable(aSource);
            return;
        case AdcCommand::ERROR_SLOTS_FULL: {
            size_t queuePos = 0;
            string qp;
            if(cmd.getParam("QP", 2, qp))
                queuePos = Util::toUInt(qp);
            noSlots(aSource, queuePos);
            return;
        }
        }
    case AdcCommand::SEV_SUCCESS:
        dcdebug("Unknown success message %s %s", err.c_str(), cmd.getParam(1).c_str());
        return;
    }
    aSource->disconnect();
}

void DownloadManager::on(UserConnectionListener::Updated, UserConnection* aSource) noexcept {
    {
        Lock l(cs);
        auto i = find(idlers.begin(), idlers.end(), aSource);
        if(i == idlers.end())
            return;
        idlers.erase(i);
    }

    checkDownloads(aSource);
}

void DownloadManager::fileNotAvailable(UserConnection* aSource) {
    Download* d = aSource->getDownload();
    if(!d) {
        dcdebug("DM::fileNotAvailable no active download, disconnecting\n");
        aSource->disconnect();
        return;
    }

    dcdebug("File Not Available: %s\n", d->getPath().c_str());

    removeDownload(d);
    fire(DownloadManagerListener::Failed(), d, str(F_("%1%: File not available") % Util::addBrackets(d->getTargetFileName())));

    QueueManager::getInstance()->removeSource(d->getPath(), aSource->getUser(), d->getType() == Transfer::TYPE_TREE ? QueueItem::Source::FLAG_NO_TREE : QueueItem::Source::FLAG_FILE_NOT_AVAILABLE, false);

    QueueManager::getInstance()->putDownload(d, false);

    checkDownloads(aSource);
}

} // namespace dcpp
