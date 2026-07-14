/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "FavoriteManager.h"

#include "File.h"
#include "HttpConnection.h"
#include "Util.h"
#include "format.h"

namespace dcpp {

void FavoriteManager::setHubList(int aHubList) {
    lastServer = aHubList;
    refresh();
}

void FavoriteManager::refresh(bool forceDownload /* = false */) {
    StringList sl = getHubLists();
    if(sl.empty()) {
        fire(FavoriteManagerListener::DownloadFailed(), Util::emptyString);
        return;
    }

    publicListServer = sl[(lastServer) % sl.size()];
    if(Util::findSubString(publicListServer, "http://") != 0 && Util::findSubString(publicListServer, "https://") != 0) {
        lastServer++;
        fire(FavoriteManagerListener::DownloadFailed(), str(F_("Invalid URL: %1%") % Util::addBrackets(publicListServer)));
        return;
    }

    if(!forceDownload) {
        string path = Util::getHubListsPath() + Util::validateFileName(publicListServer);
        if(File::getSize(path) > 0) {
            useHttp = false;
            string fileDate;
            {
                Lock l(cs);
                publicListMatrix[publicListServer].clear();
            }
            listType = (Util::stricmp(path.substr(path.size() - 4), ".bz2") == 0) ? TYPE_BZIP2 : TYPE_NORMAL;
            try {
                File cached(path, File::READ, File::OPEN);
                downloadBuf = cached.read();
                char dateBuf[20];
                time_t fd = cached.getLastModified();
                if (strftime(dateBuf, 20, "%x", localtime(&fd))) {
                    fileDate = string(dateBuf);
                }
            } catch(const FileException&) {
                downloadBuf = Util::emptyString;
            }
            if(!downloadBuf.empty()) {
                if (onHttpFinished(false)) {
                    fire(FavoriteManagerListener::LoadedFromCache(), publicListServer, fileDate);
                }
                return;
            }
        }
    }

    if(!running) {
        useHttp = true;
        {
            Lock l(cs);
            publicListMatrix[publicListServer].clear();
        }
        fire(FavoriteManagerListener::DownloadStarting(), publicListServer);
        if(c == NULL)
            c = new HttpConnection();
        c->addListener(this);
        c->downloadFile(publicListServer);
        running = true;
    }
}

void FavoriteManager::on(Data, HttpConnection*, const uint8_t* buf, size_t len) noexcept {
    if(useHttp)
        downloadBuf.append((const char*)buf, len);
}

void FavoriteManager::on(Failed, HttpConnection*, const string& aLine) noexcept {
    c->removeListener(this);
    lastServer++;
    running = false;
    if(useHttp){
        downloadBuf = Util::emptyString;
        fire(FavoriteManagerListener::DownloadFailed(), aLine);
    }
}

void FavoriteManager::on(Complete, HttpConnection*, const string& aLine) noexcept {
    bool parseSuccess = false;

    c->removeListener(this);
    if(useHttp) {
        parseSuccess = onHttpFinished(true);
    }
    running = false;
    if(parseSuccess) {
        fire(FavoriteManagerListener::DownloadFinished(), aLine);
    }
}

void FavoriteManager::on(Retried, HttpConnection*, const bool Connected) noexcept {
    if (Connected)
        downloadBuf = Util::emptyString;
}

void FavoriteManager::on(Redirected, HttpConnection*, const string& aLine) noexcept {
    if(useHttp)
        fire(FavoriteManagerListener::DownloadStarting(), aLine);
}

void FavoriteManager::on(TypeNormal, HttpConnection*) noexcept {
    if(useHttp)
        listType = TYPE_NORMAL;
}

void FavoriteManager::on(TypeBZ2, HttpConnection*) noexcept {
    if(useHttp)
        listType = TYPE_BZIP2;
}

} // namespace dcpp
