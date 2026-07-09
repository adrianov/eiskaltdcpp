/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "User.h"
#include "FastAlloc.h"
#include "MerkleTree.h"
#include "Flags.h"
#include "forward.h"
#include "Segment.h"
#include "Util.h"
#include "GetSet.h"
#include "HintedUser.h"
#include "QueueItemSource.h"

namespace dcpp {

class QueueManager;

class QueueItem : public Flags, public FastAlloc<QueueItem> {
public:
    typedef QueueItem* Ptr;
    typedef deque<Ptr> List;
    typedef List::iterator Iter;
    typedef unordered_map<string*, Ptr, CaseStringHash, CaseStringEq> StringMap;
    typedef StringMap::iterator StringIter;
    typedef unordered_map<UserPtr, Ptr, User::Hash> UserMap;
    typedef UserMap::iterator UserIter;
    typedef unordered_map<UserPtr, List, User::Hash> UserListMap;
    typedef UserListMap::iterator UserListIter;

    enum Priority {
        DEFAULT = -1,
        PAUSED = 0,
        LOWEST,
        LOW,
        NORMAL,
        HIGH,
        HIGHEST,
        LAST
    };

    enum FileFlags {
        FLAG_NORMAL = 0x00,
        FLAG_USER_LIST = 0x02,
        FLAG_DIRECTORY_DOWNLOAD = 0x04,
        FLAG_CLIENT_VIEW = 0x08,
        FLAG_TEXT = 0x20,
        FLAG_MATCH_QUEUE = 0x80,
        FLAG_XML_BZLIST = 0x100,
        FLAG_PARTIAL_LIST = 0x200
    };

    typedef QueuePartialSource PartialSource;
    typedef QueueSource Source;

    typedef std::vector<Source> SourceList;
    typedef SourceList::iterator SourceIter;
    typedef SourceList::const_iterator SourceConstIter;

    typedef set<Segment> SegmentSet;
    typedef SegmentSet::iterator SegmentIter;
    typedef SegmentSet::const_iterator SegmentConstIter;

    QueueItem(const string& aTarget, int64_t aSize, Priority aPriority, int aFlag,
              time_t aAdded, const TTHValue& tth) :
        Flags(aFlag), target(aTarget), size(aSize),
        priority(aPriority), added(aAdded), tthRoot(tth), nextPublishingTime(0)
    { }

    QueueItem(const QueueItem& rhs) :
        Flags(rhs), done(rhs.done), downloads(rhs.downloads), target(rhs.target),
        size(rhs.size), priority(rhs.priority), added(rhs.added), tthRoot(rhs.tthRoot),
        nextPublishingTime(rhs.nextPublishingTime), sources(rhs.sources), badSources(rhs.badSources),
        tempTarget(rhs.tempTarget)
    { }

    virtual ~QueueItem() { }

    int countOnlineUsers() const;
    bool hasOnlineUsers() const { return countOnlineUsers() > 0; }
    void getOnlineUsers(HintedUserList& l) const;

    SourceList& getSources() { return sources; }
    const SourceList& getSources() const { return sources; }
    SourceList& getBadSources() { return badSources; }
    const SourceList& getBadSources() const { return badSources; }

    string getTargetFileName() const { return Util::getFileName(getTarget()); }

    SourceIter getSource(const UserPtr& aUser) { return find(sources.begin(), sources.end(), aUser); }
    SourceIter getBadSource(const UserPtr& aUser) { return find(badSources.begin(), badSources.end(), aUser); }
    SourceConstIter getSource(const UserPtr& aUser) const { return find(sources.begin(), sources.end(), aUser); }
    SourceConstIter getBadSource(const UserPtr& aUser) const { return find(badSources.begin(), badSources.end(), aUser); }

    bool isSource(const UserPtr& aUser) const { return getSource(aUser) != sources.end(); }
    bool isBadSource(const UserPtr& aUser) const { return getBadSource(aUser) != badSources.end(); }
    bool isBadSourceExcept(const UserPtr& aUser, Flags::MaskType exceptions) const {
        auto i = getBadSource(aUser);
        if(i != badSources.end())
            return i->isAnySet(exceptions^Source::FLAG_MASK);
        return false;
    }

    int64_t getDownloadedBytes() const;
    double getDownloadedFraction() const { return static_cast<double>(getDownloadedBytes()) / getSize(); }

    DownloadList& getDownloads() { return downloads; }

    bool isChunkDownloaded(int64_t startPos, int64_t& len) const {
        if(len <= 0) return false;

        for(SegmentSet::const_iterator i = done.begin(); i != done.end(); ++i) {
            int64_t first  = (*i).getStart();
            int64_t second = (*i).getEnd();

            if(first <= startPos && startPos < second){
                len = min(len, second - startPos);
                return true;
            }
        }

        return false;
    }

    Segment getNextSegment(int64_t blockSize, int64_t wantedSize, int64_t lastSpeed, const PartialSource::Ptr partialSource) const;
    bool isNeededPart(const PartsInfo& partsInfo, int64_t blockSize) const;
    void getPartialInfo(PartsInfo& partialInfo, int64_t blockSize) const;

    void addSegment(const Segment& segment);
    void resetDownloaded() { done.clear(); }

    bool isFinished() const {
        return done.size() == 1 && *done.begin() == Segment(0, getSize());
    }

    bool isRunning() const {
        return !isWaiting();
    }
    bool isWaiting() const {
        return downloads.empty();
    }

    string getListName() const;

    const string& getTempTarget();
    void setTempTarget(const string& aTempTarget) { tempTarget = aTempTarget; }

    GETSET(SegmentSet, done, Done);
    GETSET(DownloadList, downloads, Downloads);
    GETSET(string, target, Target);
    GETSET(int64_t, size, Size);
    GETSET(Priority, priority, Priority);
    GETSET(time_t, added, Added);
    GETSET(TTHValue, tthRoot, TTH);
    GETSET(uint64_t, nextPublishingTime, NextPublishingTime);
private:
    QueueItem& operator=(const QueueItem&);

    friend class QueueManager;
    friend class UserQueue;
    SourceList sources;
    SourceList badSources;
    string tempTarget;

    void addSource(const HintedUser& aUser);
    void removeSource(const UserPtr& aUser, int reason);
};

} // namespace dcpp
