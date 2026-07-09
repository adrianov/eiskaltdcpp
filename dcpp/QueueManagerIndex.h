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

#pragma once

#include <deque>
#include <vector>

#include "QueueItem.h"
#include "User.h"

namespace dcpp {

class Download;

using std::deque;
using std::vector;

class DirectoryItem {
public:
    typedef DirectoryItem* Ptr;
    typedef unordered_multimap<UserPtr, Ptr, User::Hash> DirectoryMap;
    typedef DirectoryMap::iterator DirectoryIter;
    typedef pair<DirectoryIter, DirectoryIter> DirectoryPair;

    typedef vector<Ptr> List;
    typedef List::iterator Iter;

    DirectoryItem() : priority(QueueItem::DEFAULT) { }
    DirectoryItem(const UserPtr& aUser, const string& aName, const string& aTarget,
                  QueueItem::Priority p) : name(aName), target(aTarget), priority(p), user(aUser) { }
    ~DirectoryItem() { }

    UserPtr& getUser() { return user; }
    void setUser(const UserPtr& aUser) { user = aUser; }

    GETSET(string, name, Name);
    GETSET(string, target, Target);
    GETSET(QueueItem::Priority, priority, Priority);
private:
    UserPtr user;
};

typedef deque<QueueItemPtr> QueueItemList;
typedef vector<pair<QueueItem::SourceConstIter, const QueueItem*> > PFSSourceList;

/** All queue items by target */
class FileQueue {
public:
    FileQueue() : lastInsert(queue.end()) { }
    ~FileQueue() {
        for(auto& i : queue)
            delete i.second;
    }
    void add(QueueItem* qi);
    QueueItem* add(const string& aTarget, int64_t aSize, int aFlags, QueueItem::Priority p,
                   const string& aTempTarget, time_t aAdded, const TTHValue& root);

    QueueItem* find(const string& target);
    QueueItemList find(const TTHValue& tth);
    void findPFSSources(PFSSourceList&);

#ifdef WITH_DHT
    TTHValue* findPFSPubTTH();
#endif

    size_t getSize() { return queue.size(); }
    QueueItem::StringMap& getQueue() { return queue; }
    void move(QueueItem* qi, const string& aTarget);
    void remove(QueueItem* qi);
private:
    QueueItem::StringMap queue;
    QueueItem::StringIter lastInsert;
};

/** All queue items indexed by user (cache for FileQueue) */
class UserQueue {
public:
    void add(QueueItem* qi);
    void add(QueueItem* qi, const UserPtr& aUser);
    QueueItem* getNext(const UserPtr& aUser, QueueItem::Priority minPrio = QueueItem::LOWEST,
                       int64_t wantedSize = 0, int64_t lastSpeed = 0, bool allowRemove = true);
    QueueItem* getRunning(const UserPtr& aUser);
    void addDownload(QueueItem* qi, Download* d);
    void removeDownload(QueueItem* qi, const UserPtr& d);
    QueueItem::UserListMap& getList(int p) { return userQueue[p]; }
    void remove(QueueItem* qi, bool removeRunning = true);
    void remove(QueueItem* qi, const UserPtr& aUser, bool removeRunning = true);
    void setPriority(QueueItem* qi, QueueItem::Priority p);

    QueueItem::UserMap& getRunning() { return running; }
    bool isRunning(const UserPtr& aUser) const {
        return (running.find(aUser) != running.end());
    }
    int64_t getQueued(const UserPtr& aUser) const;
private:
    QueueItem::UserListMap userQueue[QueueItem::LAST];
    QueueItem::UserMap running;
};

} // namespace dcpp
