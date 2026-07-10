/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2018 Boris Pek <tehnick-8@yandex.ru>
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
#include "QueueManager.h"

#include "ClientManager.h"
#include "ConnectionManager.h"
#include "DirectoryListing.h"
#include "Download.h"
#include "DownloadManager.h"
#include "FilteredFile.h"
#include "FinishedItem.h"
#include "FinishedManager.h"
#include "HashManager.h"
#include "LogManager.h"
#include "PeerConnectLog.h"
#include "QueueAutoSearch.h"
#include "MerkleCheckOutputStream.h"
#include "SearchManager.h"
#include "SearchResult.h"
#include "SFVReader.h"
#include "ShareManager.h"
#include "SimpleXML.h"
#include "StringTokenizer.h"
#include "Transfer.h"
#include "UserConnection.h"
#include "version.h"
#include "ZUtils.h"

#ifdef WITH_DHT
#include "dht/IndexManager.h"
#endif
#include <climits>

#if !defined(_WIN32) && !defined(PATH_MAX) // Extra PATH_MAX check for macOS
#if defined(__linux)
#include <sys/syslimits.h>
#elif defined(__GNU__)
// Fix for GNU/Hurd, see:
// https://www.gnu.org/software/hurd/community/gsoc/project_ideas/maxpath.html
// https://insanecoding.blogspot.com/2007/11/pathmax-simply-isnt.html
// The same limitation as in Linux is used here (see <linux/limits.h>):
#define PATH_MAX 4096
#endif
#endif

#ifdef ff
#undef ff
#endif

namespace dcpp {

bool QueueManager::getQueueInfo(const UserPtr& aUser, string& aTarget, int64_t& aSize, int& aFlags) noexcept {
    Lock l(cs);
    QueueItem* qi = userQueue.getNext(aUser);
    if(!qi)
        return false;

    aTarget = qi->getTarget();
    aSize = qi->getSize();
    aFlags = qi->getFlags();

    return true;
}

QueueManager::QueueManager() :
    lastSave(0),
    queueFile(Util::getPath(Util::PATH_USER_CONFIG) + "Queue.xml"),
    rechecker(this),
    dirty(true),
    nextSearch(0),
    nextAutoSearchTTH(true)
{
    TimerManager::getInstance()->addListener(this);
    SearchManager::getInstance()->addListener(this);
    ClientManager::getInstance()->addListener(this);

    File::ensureDirectory(Util::getListPath());
}

QueueManager::~QueueManager() {
    SearchManager::getInstance()->removeListener(this);
    TimerManager::getInstance()->removeListener(this);
    ClientManager::getInstance()->removeListener(this);

    if(!BOOLSETTING(KEEP_LISTS)) {
        string path = Util::getListPath();

        std::sort(protectedFileLists.begin(), protectedFileLists.end());

        auto filelists = File::findFiles(path, "*.xml*");
        std::sort(filelists.begin(), filelists.end());
        std::for_each(filelists.begin(), std::set_difference(filelists.begin(), filelists.end(),
                                                             protectedFileLists.begin(), protectedFileLists.end(), filelists.begin()), &File::deleteFile);

        filelists = File::findFiles(path, "*.DcLst");
        std::sort(filelists.begin(), filelists.end());
        std::for_each(filelists.begin(), std::set_difference(filelists.begin(), filelists.end(),
                                                             protectedFileLists.begin(), protectedFileLists.end(), filelists.begin()), &File::deleteFile);

    }
}

bool QueueManager::getTTH(const string& name, TTHValue& tth) noexcept {
    Lock l(cs);
    QueueItem* qi = fileQueue.find(name);
    if(qi) {
        tth = qi->getTTH();
        return true;
    }
    return false;
}

void QueueManager::rechecked(QueueItem* qi) {
    fire(QueueManagerListener::RecheckDone(), qi->getTarget());
    fire(QueueManagerListener::StatusUpdated(), qi);

    setDirty();
}

void QueueManager::recheck(const string& aTarget) {
    rechecker.add(aTarget);
}

} // namespace dcpp
