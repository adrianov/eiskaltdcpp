/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "DownloadQueue.h"
#include "ShareIndex.h"
#include "WulforUtil.h"

#include "dcpp/ClientManager.h"
#include "dcpp/CID.h"
#include "dcpp/User.h"
#include "dcpp/Util.h"

#include <QSet>

using namespace dcpp;

namespace {

struct HolderView {
    QStringList nicks;
    int online = 0;
    UserList match;
};

QString liveNick(const ShareIndex::IndexUser &u, UserPtr *outUser = nullptr)
{
    if (outUser)
        *outUser = UserPtr();
    if (u.cid.size() == 39) {
        UserPtr user = ClientManager::getInstance()->findUser(CID(u.cid.toStdString()));
        if (user) {
            if (outUser)
                *outUser = user;
            const StringList nicks = ClientManager::getInstance()->getNicks(user->getCID(), Util::emptyString);
            if (!nicks.empty())
                return _q(nicks[0]);
        }
    }
    return u.nick;
}

QList<ShareIndex::IndexUser> filterSize(const QList<ShareIndex::IndexUser> &holders, qint64 size)
{
    if (size <= 0)
        return holders;
    QList<ShareIndex::IndexUser> out;
    for (const auto &u : holders) {
        if (u.size == size)
            out.append(u);
    }
    return out;
}

HolderView buildView(const QList<ShareIndex::IndexUser> &holders)
{
    HolderView view;
    QSet<QString> seenCid;
    for (const auto &u : holders) {
        if (u.cid.isEmpty() || seenCid.contains(u.cid))
            continue;
        seenCid.insert(u.cid);

        UserPtr user;
        const QString nick = liveNick(u, &user);
        if (!nick.isEmpty() && !view.nicks.contains(nick))
            view.nicks << nick;

        if (user && user->isOnline()) {
            ++view.online;
            view.match.push_back(user);
        }
    }
    return view;
}

void applyView(QVariantMap &map, const HolderView &view)
{
    if (view.nicks.isEmpty())
        return;

    QStringList nicks = view.nicks;
    const QString existing = map.value("USERS").toString();
    if (!existing.isEmpty() && existing != DownloadQueue::tr("No users...")) {
        for (const QString &nick : existing.split(QStringLiteral(", "))) {
            if (!nick.isEmpty() && !nicks.contains(nick))
                nicks << nick;
        }
    }
    map["USERS"] = nicks.join(QStringLiteral(", "));

    if (map.value("STATUS").toString() == DownloadQueue::tr("Running..."))
        return;

    const int online = qMin(qMax(view.online, map.value("SRC_ONLINE").toInt()), nicks.size());
    map["STATUS"] = DownloadQueue::tr("%1 of %2 user(s) online").arg(online).arg(nicks.size());
}

void matchHolders(ShareIndex *idx, const UserList &users)
{
    if (idx && !users.empty())
        idx->matchQueue(users);
}

} // namespace

void DownloadQueue::applyIndexUsers(VarMap &map, bool attach)
{
    ShareIndex *idx = ShareIndex::getInstance();
    if (!idx || !idx->isOpen())
        return;

    const QString tth = map.value("TTH").toString();
    if (tth.size() != 39)
        return;

    const qint64 size = map.value("ESIZE").toLongLong();
    const HolderView view = buildView(idx->usersByTth(QStringList{tth}, size > 0 ? size : 0).value(tth));
    applyView(map, view);
    if (attach)
        matchHolders(idx, view.match);
}

void DownloadQueue::applyIndexUsers(QList<VarMap> &rows, bool attach)
{
    ShareIndex *idx = ShareIndex::getInstance();
    if (!idx || !idx->isOpen() || rows.isEmpty())
        return;

    QStringList tths;
    tths.reserve(rows.size());
    for (const VarMap &map : rows) {
        const QString tth = map.value("TTH").toString();
        if (tth.size() == 39)
            tths << tth;
    }
    if (tths.isEmpty())
        return;

    const auto byTth = idx->usersByTth(tths, 0);
    if (byTth.isEmpty())
        return;

    UserList match;
    QSet<QString> matchCid;
    for (VarMap &map : rows) {
        const QString tth = map.value("TTH").toString();
        const qint64 size = map.value("ESIZE").toLongLong();
        const HolderView view = buildView(filterSize(byTth.value(tth), size));
        applyView(map, view);
        if (!attach)
            continue;
        for (const UserPtr &user : view.match) {
            const QString cid = _q(user->getCID().toBase32());
            if (matchCid.contains(cid))
                continue;
            matchCid.insert(cid);
            match.push_back(user);
        }
    }
    if (attach)
        matchHolders(idx, match);
}
