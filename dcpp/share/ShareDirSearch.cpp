/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "ShareManager.h"

#include "AdcHub.h"
#include "SearchResult.h"
#include "StringTokenizer.h"
#include "Text.h"
#include "Util.h"
#include "share/ShareFileType.h"

#include <algorithm>
#include <limits>

namespace dcpp {

using std::numeric_limits;
using std::remove;

namespace {

void bumpHits() {
    ShareManager::getInstance()->addHits(1);
}

bool nmdcSizeOk(int searchType, int64_t want, int64_t have) {
    if(searchType == SearchManager::SIZE_ATLEAST)
        return want <= have;
    if(searchType == SearchManager::SIZE_ATMOST)
        return want >= have;
    return true;
}

bool matchesAll(const StringSearch::List& terms, const string& name) {
    for(auto& term : terms) {
        if(!term.match(name))
            return false;
    }
    return true;
}

StringSearch::List* narrowTerms(const string& dirName, StringSearch::List& terms,
                                unique_ptr<StringSearch::List>& owned) {
    for(auto& term : terms) {
        if(!term.match(dirName))
            continue;
        if(!owned)
            owned.reset(new StringSearch::List(terms));
        owned->erase(remove(owned->begin(), owned->end(), term), owned->end());
    }
    return owned ? owned.get() : &terms;
}

void addDirResult(SearchResultList& results, int64_t size, const string& path) {
    results.push_back(SearchResultPtr(new SearchResult(SearchResult::TYPE_DIRECTORY, size, path, TTHValue())));
    bumpHits();
}

void addFileResult(SearchResultList& results, int64_t size, const string& path, const TTHValue& tth) {
    results.push_back(SearchResultPtr(new SearchResult(SearchResult::TYPE_FILE, size, path, tth)));
    bumpHits();
}

inline uint16_t toCode(char a, char b) {
    return static_cast<uint16_t>(a) | (static_cast<uint16_t>(b) << 8);
}

} // namespace

void ShareManager::Directory::search(SearchResultList& aResults, StringSearch::List& aStrings,
        int aSearchType, int64_t aSize, int aFileType, Client*, StringList::size_type maxResults) const noexcept {
    if(!hasType(aFileType))
        return;

    unique_ptr<StringSearch::List> narrowed;
    StringSearch::List* cur = narrowTerms(name, aStrings, narrowed);

    const bool sizeOk = (aSearchType != SearchManager::SIZE_ATLEAST) || (aSize == 0);
    if(cur->empty() && sizeOk
            && (aFileType == SearchManager::TYPE_ANY || aFileType == SearchManager::TYPE_DIRECTORY))
        addDirResult(aResults, 0, getFullName());

    if(aFileType != SearchManager::TYPE_DIRECTORY) {
        for(auto& file : files) {
            if(!nmdcSizeOk(aSearchType, aSize, file.getSize()))
                continue;
            if(!matchesAll(*cur, file.getName()))
                continue;
            if(!ShareFileType::matches(file.getName(), aFileType))
                continue;
            addFileResult(aResults, file.getSize(), getFullName() + file.getName(), file.getTTH());
            if(aResults.size() >= maxResults)
                break;
        }
    }

    for(auto i = directories.begin(); i != directories.end() && aResults.size() < maxResults; ++i)
        i->second->search(aResults, *cur, aSearchType, aSize, aFileType, nullptr, maxResults);
}

void ShareManager::search(SearchResultList& results, const string& aString, int aSearchType,
        int64_t aSize, int aFileType, Client* aClient, StringList::size_type maxResults) noexcept {
    Lock l(cs);
    if(aFileType == SearchManager::TYPE_TTH) {
        if(aString.compare(0, 4, "TTH:") == 0) {
            auto i = tthIndex.find(TTHValue(aString.substr(4)));
            if(i != tthIndex.end()) {
                addFileResult(results, i->second->getSize(),
                        i->second->getParent()->getFullName() + i->second->getName(), i->second->getTTH());
            }
        }
        return;
    }

    StringTokenizer<string> t(Text::toLower(aString), '$');
    StringList& sl = t.getTokens();
    if(!bloom.match(sl))
        return;

    StringSearch::List ssl;
    for(auto& token : sl) {
        if(!token.empty())
            ssl.push_back(StringSearch(token));
    }
    if(ssl.empty())
        return;

    for(auto j = directories.begin(); j != directories.end() && results.size() < maxResults; ++j)
        (*j)->search(results, ssl, aSearchType, aSize, aFileType, aClient, maxResults);
}

ShareManager::AdcSearch::AdcSearch(const StringList& adcParams) :
    include(&includeInit),
    gt(0),
    lt(numeric_limits<int64_t>::max()),
    hasRoot(false),
    isDirectory(false)
{
    for(auto& p: adcParams) {
        if(p.size() <= 2)
            continue;

        const auto cmd = toCode(p[0], p[1]);
        if(cmd == toCode('T', 'R')) {
            hasRoot = true;
            root = TTHValue(p.substr(2));
            return;
        }
        if(cmd == toCode('A', 'N'))
            includeInit.emplace_back(p.substr(2));
        else if(cmd == toCode('N', 'O'))
            exclude.emplace_back(p.substr(2));
        else if(cmd == toCode('E', 'X'))
            ext.push_back(p.substr(2));
        else if(cmd == toCode('G', 'R')) {
            auto exts = AdcHub::parseSearchExts(Util::toInt(p.substr(2)));
            ext.insert(ext.begin(), exts.begin(), exts.end());
        } else if(cmd == toCode('R', 'X'))
            noExt.push_back(p.substr(2));
        else if(cmd == toCode('G', 'E'))
            gt = Util::toInt64(p.substr(2));
        else if(cmd == toCode('L', 'E'))
            lt = Util::toInt64(p.substr(2));
        else if(cmd == toCode('E', 'Q'))
            lt = gt = Util::toInt64(p.substr(2));
        else if(cmd == toCode('T', 'Y'))
            isDirectory = (p[2] == '2');
    }
}

bool ShareManager::AdcSearch::isExcluded(const string& str) {
    for(auto& i : exclude) {
        if(i.match(str))
            return true;
    }
    return false;
}

bool ShareManager::AdcSearch::hasExt(const string& name) {
    if(ext.empty())
        return true;
    if(!noExt.empty()) {
        ext = StringList(ext.begin(), set_difference(ext.begin(), ext.end(), noExt.begin(), noExt.end(), ext.begin()));
        noExt.clear();
    }
    for(auto& i : ext) {
        if(name.length() >= i.length() && Util::stricmp(name.c_str() + name.length() - i.length(), i.c_str()) == 0)
            return true;
    }
    return false;
}

void ShareManager::Directory::searchAdcFiles(SearchResultList& results, AdcSearch& query,
        StringSearch::List* terms, StringList::size_type maxResults) const noexcept {
    for(auto& file : files) {
        if(file.getSize() < query.gt || file.getSize() > query.lt)
            continue;
        if(query.isExcluded(file.getName()) || !matchesAll(*terms, file.getName()))
            continue;
        if(!query.hasExt(file.getName()))
            continue;
        addFileResult(results, file.getSize(), getFullName() + file.getName(), file.getTTH());
        if(results.size() >= maxResults)
            return;
    }
}

void ShareManager::Directory::search(SearchResultList& aResults, AdcSearch& aStrings,
        StringList::size_type maxResults) const noexcept {
    StringSearch::List* old = aStrings.include;
    unique_ptr<StringSearch::List> narrowed;
    StringSearch::List* cur = aStrings.include;
    for(auto i = cur->begin(); i != cur->end(); ++i) {
        if(!i->match(name) || aStrings.isExcluded(name))
            continue;
        if(!narrowed)
            narrowed.reset(new StringSearch::List(*cur));
        narrowed->erase(remove(narrowed->begin(), narrowed->end(), *i), narrowed->end());
    }
    if(narrowed)
        cur = narrowed.get();
    aStrings.include = cur;

    if(cur->empty() && aStrings.ext.empty() && aStrings.gt == 0)
        addDirResult(aResults, getSize(), getFullName());

    if(!aStrings.isDirectory)
        searchAdcFiles(aResults, aStrings, cur, maxResults);

    if(aResults.size() < maxResults) {
        for(auto i = directories.begin(); i != directories.end() && aResults.size() < maxResults; ++i)
            i->second->search(aResults, aStrings, maxResults);
    }
    aStrings.include = old;
}

void ShareManager::search(SearchResultList& results, const StringList& params,
        StringList::size_type maxResults) noexcept {
    AdcSearch srch(params);
    Lock l(cs);

    if(srch.hasRoot) {
        auto i = tthIndex.find(srch.root);
        if(i != tthIndex.end()) {
            addFileResult(results, i->second->getSize(),
                    i->second->getParent()->getFullName() + i->second->getName(), i->second->getTTH());
        }
        return;
    }

    for(auto& term : srch.includeInit) {
        if(!bloom.match(term.getPattern()))
            return;
    }

    for(auto j = directories.begin(); j != directories.end() && results.size() < maxResults; ++j)
        (*j)->search(results, srch, maxResults);
}

} // namespace dcpp
