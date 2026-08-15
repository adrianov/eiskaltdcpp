/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "filebrowser/FilterMatch.h"
#include "SearchFileTypes.h"

#include "dcpp/stdinc.h"
#include "dcpp/SearchManager.h"

bool FilterMatch::needTth() const
{
    for (const Term &term : terms) {
        if (!term.exclude && term.value.size() >= 39)
            return true;
    }
    return false;
}

bool FilterMatch::matchesText(const QString &haystack) const
{
    for (const Term &term : terms) {
        const bool found = haystack.contains(term.value, Qt::CaseInsensitive);
        if (term.exclude ? found : !found)
            return false;
    }
    return true;
}

bool FilterMatch::isActive() const
{
    const bool noText = terms.empty();
    const bool noType = !dirsOnly && !filesOnly && extFilter.isEmpty() && !adultVideo;
    const bool noSize = !sizeLimit || sizeMode == dcpp::SearchManager::SIZE_DONTCARE;
    return !(noText && noType && noSize);
}

void FilterMatch::setTerms(const QStringList &raw)
{
    terms.clear();
    terms.reserve(static_cast<size_t>(raw.size()));
    for (const QString &term : raw) {
        if (term.isEmpty())
            continue;
        if (term.at(0) == QLatin1Char('-')) {
            if (term.size() == 1)
                continue;
            terms.push_back({term.mid(1), true});
        } else {
            terms.push_back({term, false});
        }
    }
}

bool FilterMatch::sizeOk(qulonglong size) const
{
    if (!sizeLimit || sizeMode == dcpp::SearchManager::SIZE_DONTCARE)
        return true;
    if (sizeMode == dcpp::SearchManager::SIZE_ATLEAST)
        return size >= sizeLimit;
    if (sizeMode == dcpp::SearchManager::SIZE_ATMOST)
        return size <= sizeLimit;
    return true;
}

bool FilterMatch::dirPasses(const QString &path, qulonglong size) const
{
    if (!dirsOnly || !sizeOk(size))
        return false;
    return terms.empty() || matchesText(path);
}

bool FilterMatch::acceptFile(const QString &name, const QString &path, const QString &tth,
                             qulonglong size) const
{
    if (dirsOnly)
        return false;

    if (!SearchFileTypes::matchesFile(name, path, extFilter, adultVideo))
        return false;

    if (!sizeOk(size))
        return false;

    if (terms.empty())
        return true;

    QString hay = joinPath(path, name);
    if (!tth.isEmpty()) {
        hay += QLatin1Char(' ');
        hay += tth;
    }
    return matchesText(hay);
}
