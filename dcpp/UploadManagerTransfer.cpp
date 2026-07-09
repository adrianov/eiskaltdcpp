/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "UploadManager.h"

#include "ConnectionManager.h"
#include "LogManager.h"
#include "ShareManager.h"
#include "FilteredFile.h"
#include "ZUtils.h"
#include "AdcCommand.h"
#include "Upload.h"
#include "UserConnection.h"
#include "FinishedManager.h"
#include "File.h"

namespace dcpp {

void UploadManager::on(UserConnectionListener::Get, UserConnection* aSource, const string& aFile, int64_t aResume) noexcept {
    if(aSource->getState() != UserConnection::STATE_GET) {
        dcdebug("UM::onGet Bad state, ignoring\n");
        return;
    }

    if(prepareFile(*aSource, Transfer::names[Transfer::TYPE_FILE], Util::toAdcFile(aFile), aResume, -1)) {
        if(aResume == 0)
            aSource->resetTransferSpeed();
        aSource->setState(UserConnection::STATE_SEND);
        aSource->fileLength(Util::toString(aSource->getUpload()->getSize()));
    }
}

void UploadManager::on(UserConnectionListener::Send, UserConnection* aSource) noexcept {
    if(aSource->getState() != UserConnection::STATE_SEND) {
        dcdebug("UM::onSend Bad state, ignoring\n");
        return;
    }

    Upload* u = aSource->getUpload();
    dcassert(u != NULL);

    u->setStart(GET_TICK());
    u->tick();
    aSource->setState(UserConnection::STATE_RUNNING);
    aSource->transmitFile(u->getStream());
    fire(UploadManagerListener::Starting(), u);
}

void UploadManager::on(AdcCommand::GET, UserConnection* aSource, const AdcCommand& c) noexcept {
    if(aSource->getState() != UserConnection::STATE_GET) {
        dcdebug("UM::onGET Bad state, ignoring\n");
        return;
    }

    const string& type = c.getParam(0);
    const string& fname = c.getParam(1);
    int64_t aStartPos = Util::toInt64(c.getParam(2));
    int64_t aBytes = Util::toInt64(c.getParam(3));

    if(prepareFile(*aSource, type, fname, aStartPos, aBytes, c.hasFlag("RE", 4))) {
        if(aStartPos == 0)
            aSource->resetTransferSpeed();

        Upload* u = aSource->getUpload();
        dcassert(u != NULL);

        AdcCommand cmd(AdcCommand::CMD_SND);
        cmd.addParam(type).addParam(fname)
                .addParam(Util::toString(u->getStartPos()))
                .addParam(Util::toString(u->getSize()));

        if(c.hasFlag("ZL", 4)) {
            u->setStream(new FilteredInputStream<ZFilter, true>(u->getStream()));
            u->setFlag(Upload::FLAG_ZUPLOAD);
            cmd.addParam("ZL1");
        }

        aSource->send(cmd);

        u->setStart(GET_TICK());
        u->tick();
        aSource->setState(UserConnection::STATE_RUNNING);
        aSource->transmitFile(u->getStream());
        fire(UploadManagerListener::Starting(), u);
    }
}

void UploadManager::on(UserConnectionListener::BytesSent, UserConnection* aSource, size_t aBytes, size_t aActual) noexcept {
    dcassert(aSource->getState() == UserConnection::STATE_RUNNING);
    Upload* u = aSource->getUpload();
    dcassert(u != NULL);
    u->addPos(aBytes, aActual);
    u->tick();
    aSource->updateTransferSpeed(u->getStartPos() + u->getPos());
}

void UploadManager::on(UserConnectionListener::Failed, UserConnection* aSource, const string& aError) noexcept {
    Upload* u = aSource->getUpload();

    if(u) {
        fire(UploadManagerListener::Failed(), u, aError);

        dcdebug("UM::onFailed (%s): Removing upload\n", aError.c_str());
        removeUpload(u);
    }

    removeConnection(aSource);
}

void UploadManager::on(UserConnectionListener::TransmitDone, UserConnection* aSource) noexcept {
    dcassert(aSource->getState() == UserConnection::STATE_RUNNING);
    Upload* u = aSource->getUpload();
    dcassert(u != NULL);

    aSource->setState(UserConnection::STATE_GET);

    if(BOOLSETTING(LOG_UPLOADS) && u->getType() != Transfer::TYPE_TREE && (BOOLSETTING(LOG_FILELIST_TRANSFERS) || u->getType() != Transfer::TYPE_FULL_LIST)) {
        StringMap params;
        u->getParams(*aSource, params);
        LOG(LogManager::UPLOAD, params);
    }

    fire(UploadManagerListener::Complete(), u);
    removeUpload(u);
}

} // namespace dcpp
