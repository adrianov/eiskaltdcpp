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
#include "FilteredFile.h"
#include "ZUtils.h"
#include "HashManager.h"
#include "CryptoManager.h"
#include "Upload.h"
#include "UserConnection.h"
#include "File.h"
#include "FavoriteManager.h"
#include "ClientManager.h"
#include "LogManager.h"

namespace dcpp {

bool UploadManager::prepareFile(UserConnection& aSource, const string& aType, const string& aFile, int64_t aStartPos, int64_t aBytes, bool listRecursive) {
    dcdebug("Preparing %s %s " I64_FMT " " I64_FMT " %d\n", aType.c_str(), aFile.c_str(),
            static_cast<long long int>(aStartPos), static_cast<long long int>(aBytes), listRecursive);

    if(aFile.empty() || aStartPos < 0 || aBytes < -1 || aBytes == 0) {
        aSource.fileNotAvail("Invalid request");
        return false;
    }

    InputStream* is = 0;
    int64_t start = 0;
    int64_t size = 0;
    int64_t fileSize = 0;

    bool userlist = (aFile == Transfer::USER_LIST_NAME_BZ || aFile == Transfer::USER_LIST_NAME);
    bool free = userlist;

    string sourceFile;
    Transfer::Type type;

    try {
        if(aType == Transfer::names[Transfer::TYPE_FILE]) {
            sourceFile = ShareManager::getInstance()->toReal(aFile);

            if(aFile == Transfer::USER_LIST_NAME) {
                // Unpack before sending...
                string bz2 = File(sourceFile, File::READ, File::OPEN).read();
                string xml;
                CryptoManager::getInstance()->decodeBZ2(reinterpret_cast<const uint8_t*>(bz2.data()), bz2.size(), xml);
                // Clear to save some memory...
                string().swap(bz2);
                is = new MemoryInputStream(xml);
                start = 0;
                fileSize = size = xml.size();
            } else {
                {
                    ShareManager *SM = ShareManager::getInstance();
                    string msg;
                    if ( aFile != Transfer::USER_LIST_NAME_BZ && aFile != Transfer::USER_LIST_NAME &&
                         !limits.IsUserAllowed(SM->toVirtual(SM->getTTH(aFile)), aSource.getUser(), &msg)
                         )
                    {
                        throw ShareException(msg);
                    }
                    else if ( aFile != Transfer::USER_LIST_NAME_BZ && aFile != Transfer::USER_LIST_NAME &&
                              hasUpload(aSource)
                              )
                    {
                        msg = _("Connection already exists.");

                        throw ShareException(msg);
                    }
                }
                File* f = new File(sourceFile, File::READ, File::OPEN);

                start = aStartPos;
                int64_t sz = f->getSize();
                size = (aBytes == -1) ? sz - start : aBytes;
                fileSize = sz;

                if((start + size) > sz) {
                    aSource.fileNotAvail();
                    delete f;
                    return false;
                }

                free = free || (sz <= (int64_t)(SETTING(SET_MINISLOT_SIZE) * 1024) );

                f->setPos(start);
                is = f;
                if((start + size) < sz) {
                    is = new LimitedInputStream<true>(is, size);
                }
            }
            type = userlist ? Transfer::TYPE_FULL_LIST : Transfer::TYPE_FILE;
        } else if(aType == Transfer::names[Transfer::TYPE_TREE]) {
            sourceFile = ShareManager::getInstance()->toReal(aFile);
            MemoryInputStream* mis = ShareManager::getInstance()->getTree(aFile);
            if(!mis) {
                aSource.fileNotAvail();
                return false;
            }

            start = 0;
            fileSize = size = mis->getSize();
            is = mis;
            free = true;
            type = Transfer::TYPE_TREE;
        } else if(aType == Transfer::names[Transfer::TYPE_PARTIAL_LIST]) {
            // Partial file list
            MemoryInputStream* mis = ShareManager::getInstance()->generatePartialList(aFile, listRecursive);
            if(mis == NULL) {
                aSource.fileNotAvail();
                return false;
            }

            start = 0;
            fileSize = size = mis->getSize();
            is = mis;
            free = true;
            type = Transfer::TYPE_PARTIAL_LIST;
        } else {
            aSource.fileNotAvail("Unknown file type");
            return false;
        }
    } catch(const ShareException& e) {
        // Partial file sharing: serve from queue/finished downloads when not in share.
        if (aType == Transfer::names[Transfer::TYPE_FILE]) {
            const int partial = openPartialFile(aSource, aFile, aStartPos, aBytes, is, sourceFile,
                                                start, size, fileSize, type);
            if (partial > 0)
                return finishPrepare(aSource, is, sourceFile, start, size, type, free, aFile, aStartPos, aBytes);
            if (partial < 0)
                return false;
        }
        aSource.fileNotAvail(e.getError());
        return false;
    } catch(const Exception& e) {
        LogManager::getInstance()->message(str(F_("Unable to send file %1%: %2%") % Util::addBrackets(sourceFile) % e.getError()));
        aSource.fileNotAvail();
        return false;
    }

    return finishPrepare(aSource, is, sourceFile, start, size, type, free, aFile, aStartPos, aBytes);
}

int64_t UploadManager::getRunningAverage() {
    Lock l(cs);
    int64_t avg = 0;
    for(auto u: uploads) {
        avg += u->getAverageSpeed();
    }
    return avg;
}

} // namespace dcpp
