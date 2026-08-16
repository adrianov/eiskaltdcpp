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

#include "search/SearchSources.h"
#include "search/SearchItem.h"

#include "dcpp/stdinc.h"
#include "dcpp/CID.h"
#include "dcpp/ClientManager.h"
#include "dcpp/User.h"

#include <QSet>

using namespace dcpp;

namespace {

bool cidOffline(const QString &cid)
{
    if (cid.size() != 39)
        return true;
    const UserPtr user = ClientManager::getInstance()->findUser(CID(cid.toStdString()));
    return !user || !user->isOnline();
}

QString sourceIp(const SearchItem *item)
{
    QString ip = item->data(COLUMN_SF_IP).toString().trimmed();
    if (ip == QLatin1String("0.0.0.0"))
        ip.clear();
    return ip;
}

template <typename Fn>
void forEachHolder(const SearchItem *item, Fn fn)
{
    fn(item);
    for (const SearchItem *child : item->children())
        fn(child);
}

/** Merge holders of one file: IP, nick, then CID. */
class IdentityTally {
public:
    void add(const SearchItem *item)
    {
        const QString ip = sourceIp(item);
        const QString nick = item->data(COLUMN_SF_NICK).toString();
        if (!ip.isEmpty()) {
            ips.insert(ip);
            if (!nick.isEmpty())
                nicksWithIp.insert(nick);
            return;
        }
        if (!nick.isEmpty()) {
            nicksNoIp.insert(nick);
            return;
        }
        cidsOnly.insert(item->cid);
    }

    int total() const
    {
        int n = ips.size() + cidsOnly.size();
        for (const QString &nick : nicksNoIp) {
            if (!nicksWithIp.contains(nick))
                ++n;
        }
        return n;
    }

private:
    QSet<QString> ips;
    QSet<QString> nicksWithIp;
    QSet<QString> nicksNoIp;
    QSet<QString> cidsOnly;
};

int countUnique(const SearchItem *group)
{
    IdentityTally tally;
    forEachHolder(group, [&](const SearchItem *it) { tally.add(it); });
    return tally.total();
}

int countOnline(const SearchItem *group)
{
    QSet<QString> cids;
    forEachHolder(group, [&](const SearchItem *it) {
        if (!cidOffline(it->cid))
            cids.insert(it->cid);
    });
    return cids.size();
}

} // namespace

void SearchSources::invalidate()
{
    uniqueCached = -1;
    onlineCached = -1;
}

void SearchSources::invalidateOnline()
{
    onlineCached = -1;
}

int SearchSources::uniqueCount(const SearchItem *group) const
{
    if (uniqueCached < 0)
        uniqueCached = countUnique(group);
    return uniqueCached;
}

int SearchSources::onlineCount(const SearchItem *group) const
{
    if (onlineCached < 0)
        onlineCached = countOnline(group);
    return onlineCached;
}
