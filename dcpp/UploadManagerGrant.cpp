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

#include "ShareManager.h"
#include "Upload.h"
#include "UserConnection.h"
#include "FavoriteManager.h"

namespace dcpp {

bool UploadManager::finishPrepare(UserConnection& aSource, InputStream* is, const string& sourceFile,
                                  int64_t start, int64_t size, Transfer::Type type, bool free,
                                  const string& aFile, int64_t aStartPos, int64_t aBytes) {
    Lock l(cs);

    bool extraSlot = false;

    if(!aSource.isSet(UserConnection::FLAG_HASSLOT)) {
        bool hasReserved = (reservedSlots.find(aSource.getUser()) != reservedSlots.end());
        bool isFavorite = FavoriteManager::getInstance()->hasSlot(aSource.getUser());

        if(!(hasReserved || isFavorite || getFreeSlots() > 0 || getAutoSlot())) {
            bool supportsFree = aSource.isSet(UserConnection::FLAG_SUPPORTS_MINISLOTS);
            bool allowedFree = aSource.isSet(UserConnection::FLAG_HASEXTRASLOT) || aSource.isSet(UserConnection::FLAG_OP) || getFreeExtraSlots() > 0;
            if(free && supportsFree && allowedFree) {
                extraSlot = true;
            } else {
                delete is;
                aSource.maxedOut();

                // Check for tth root identifier
                string tFile = aFile;
                if (tFile.compare(0, 4, "TTH/") == 0)
                    tFile = ShareManager::getInstance()->toVirtual(TTHValue(aFile.substr(4)));

                addFailedUpload(aSource, tFile +
                                " (" +  Util::formatBytes(aStartPos) + " - " + Util::formatBytes(aStartPos + aBytes) + ")");
                aSource.disconnect();
                return false;
            }
        } else {
            clearUserFiles(aSource.getUser());  // this user is using a full slot, nix them.
        }

        setLastGrant(GET_TICK());
    }

    Upload* u = new Upload(aSource, sourceFile, TTHValue());
    u->setStream(is);
    u->setSegment(Segment(start, size));

    u->setType(type);

    uploads.push_back(u);

    if(!aSource.isSet(UserConnection::FLAG_HASSLOT)) {
        if(extraSlot) {
            if(!aSource.isSet(UserConnection::FLAG_HASEXTRASLOT)) {
                aSource.setFlag(UserConnection::FLAG_HASEXTRASLOT);
                extra++;
            }
        } else {
            if(aSource.isSet(UserConnection::FLAG_HASEXTRASLOT)) {
                aSource.unsetFlag(UserConnection::FLAG_HASEXTRASLOT);
                extra--;
            }
            aSource.setFlag(UserConnection::FLAG_HASSLOT);
            running++;
        }

        reservedSlots.erase(aSource.getUser());
    }

    return true;
}

} // namespace dcpp
