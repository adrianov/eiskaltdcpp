/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"

#include "QueueAutoSearch.h"

#include "SettingsManager.h"
#include "Util.h"

namespace dcpp {

// Bytes >= 0x80 are UTF-8 continuation/lead bytes: keep them so non-ASCII words work.
static bool isWordChar(unsigned char c) {
    return isalnum(c) || c == '_' || c >= 0x80;
}

string QueueAutoSearch::longestWord(const string& fileName) {
    string best;
    string word;
    for(size_t i = 0; i <= fileName.size(); ++i) {
        if(i < fileName.size() && isWordChar((unsigned char)fileName[i])) {
            word += fileName[i];
        } else if(!word.empty()) {
            if(word.size() > best.size())
                best = word;
            word.clear();
        }
    }
    return best;
}

static bool recentHas(const StringList& recent, const string& key) {
    return find(recent.begin(), recent.end(), key) != recent.end();
}

static QueueItem* findCandidate(QueueItem* cand, QueueItem::StringIter start, QueueItem::StringIter end,
                                const StringList& recent, QueueAutoSearch::Mode mode) {
    for(auto i = start; i != end; ++i) {
        QueueItem* q = i->second;

        if((cand != NULL) && q->isRunning())
            continue;
        if(q->isFinished())
            continue;
        if(q->isSet(QueueItem::FLAG_USER_LIST))
            continue;
        if(q->getPriority() == QueueItem::PAUSED)
            continue;
        if(q->countOnlineUsers() >= SETTING(AUTO_SEARCH_LIMIT))
            continue;

        if(mode == QueueAutoSearch::TTH) {
            if(recentHas(recent, q->getTarget()))
                continue;
        } else {
            const string word = QueueAutoSearch::longestWord(q->getTargetFileName());
            if(word.size() < 3 || recentHas(recent, word))
                continue;
        }

        cand = q;
        if(cand->isWaiting())
            break;
    }
    return cand;
}

static QueueItem* pickItem(QueueItem::StringMap& queue, const StringList& recent, QueueAutoSearch::Mode mode) {
    if(queue.empty())
        return NULL;

    QueueItem::StringMap::size_type start = (QueueItem::StringMap::size_type)Util::rand((uint32_t)queue.size());
    auto i = queue.begin();
    advance(i, start);

    QueueItem* cand = findCandidate(NULL, i, queue.end(), recent, mode);
    if(cand == NULL || cand->isRunning())
        cand = findCandidate(cand, queue.begin(), i, recent, mode);
    return cand;
}

static AutoSearchPick pick(QueueItem::StringMap& queue, const StringList& recent, QueueAutoSearch::Mode mode) {
    AutoSearchPick result;
    result.item = pickItem(queue, recent, mode);
    if(!result.item)
        return result;

    if(mode == QueueAutoSearch::TTH) {
        result.query = result.item->getTTH().toBase32();
        result.type = SearchManager::TYPE_TTH;
    } else {
        result.query = QueueAutoSearch::longestWord(result.item->getTargetFileName());
        result.type = SearchManager::TYPE_ANY;
    }
    return result;
}

static void trimRecent(StringList& list, size_t queueSize) {
    while(list.size() >= queueSize || list.size() > 30)
        list.erase(list.begin());
}

AutoSearchPick QueueAutoSearch::pickAlternating(QueueItem::StringMap& queue, StringList& recent,
                                                StringList& recentKeywords, bool preferTTH) {
    trimRecent(recent, queue.size());
    trimRecent(recentKeywords, queue.size());

    Mode mode = preferTTH ? TTH : KEYWORD;
    AutoSearchPick result = pick(queue, mode == TTH ? recent : recentKeywords, mode);
    if(!result.item) {
        // The preferred kind has no candidate: fall back so auto-search never stalls
        mode = (mode == TTH) ? KEYWORD : TTH;
        result = pick(queue, mode == TTH ? recent : recentKeywords, mode);
    }

    if(result.item) {
        if(mode == TTH)
            recent.push_back(result.item->getTarget());
        else
            recentKeywords.push_back(result.query);
    }
    return result;
}

} // namespace dcpp
