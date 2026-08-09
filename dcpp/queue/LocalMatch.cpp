/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "queue/LocalMatch.h"

#include "File.h"
#include "format.h"
#include "HashManager.h"
#include "LogManager.h"
#include "QueueManager.h"
#include "Util.h"

namespace dcpp {

namespace {

struct MatchCand {
    string target;
    int64_t size;
    TTHValue tth;
};

bool sameSize(const string& target, int64_t size) {
    return size > 0 && File::getSize(target) == size;
}

bool sameTth(const string& target, const TTHValue& tth) {
    const TTHValue* cached = HashManager::getInstance()->getFileTTHif(target);
    return cached && *cached == tth;
}

bool skipItem(const QueueItem* qi) {
    return !qi || qi->isSet(QueueItem::FLAG_USER_LIST) || qi->isSet(QueueItem::FLAG_PARTIAL_LIST)
            || !qi->getTTH();
}

void logRemoved(const string& target) {
    LogManager::getInstance()->message(str(F_("Local copy matches, removed from queue: %1%")
            % Util::addBrackets(target)));
}

} // namespace

LocalMatch::LocalMatch(QueueManager& queue) : queue(queue) {
    HashManager::getInstance()->addListener(this);
}

LocalMatch::~LocalMatch() {
    HashManager::getInstance()->removeListener(this);
}

bool LocalMatch::matches(const string& target, int64_t size, const TTHValue& tth) noexcept {
    return !target.empty() && sameSize(target, size) && sameTth(target, tth);
}

void LocalMatch::sweep(bool hashMissing) noexcept {
    vector<MatchCand> cands;
    {
        Lock l(queue.cs);
        cands.reserve(queue.fileQueue.getSize());
        for(auto& i: queue.fileQueue.getQueue()) {
            QueueItem* qi = i.second;
            if(skipItem(qi))
                continue;
            cands.push_back({ qi->getTarget(), qi->getSize(), qi->getTTH() });
        }
    }

    StringList remove;
    for(auto& c: cands) {
        if(!sameSize(c.target, c.size))
            continue;
        if(sameTth(c.target, c.tth)) {
            remove.push_back(c.target);
            continue;
        }
        if(hashMissing && !HashManager::getInstance()->getFileTTHif(c.target))
            HashManager::getInstance()->checkTTH(c.target, c.size, 0);
    }

    for(auto& target: remove) {
        logRemoved(target);
        queue.remove(target);
    }
}

void LocalMatch::on(HashManagerListener::TTHDone, const string& fileName, const TTHValue& root) noexcept {
    string target;
    {
        Lock l(queue.cs);
        QueueItem* qi = queue.fileQueue.find(fileName);
        if(skipItem(qi) || !(qi->getTTH() == root) || !sameSize(qi->getTarget(), qi->getSize()))
            return;
        target = qi->getTarget();
    }
    if(target.empty())
        return;

    logRemoved(target);
    queue.remove(target);
}

} // namespace dcpp
