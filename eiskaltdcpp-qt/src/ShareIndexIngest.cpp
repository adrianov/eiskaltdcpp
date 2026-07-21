/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ShareIndex.h"

#ifdef USE_QT_SQLITE

#include "ShareIndexQueueCore.h"
#include "WulforUtil.h"
#include "dcpp/ClientManager.h"
#include "dcpp/PeerConnectHub.h"
#include "dcpp/Util.h"

#include <QElapsedTimer>

using namespace dcpp;
using namespace ShareIndexWriteQueue;

namespace {

QString joinHubField(const StringList &parts)
{
    if (parts.empty())
        return QString();
    return _q(Util::toString(parts));
}

void fillUserHubFields(const UserPtr &user, QString &hubUrl, QString &hubName, QString &ip)
{
    if (!user)
        return;
    ClientManager *cm = ClientManager::getInstance();
    if (!cm)
        return;

    const string hint = _tq(hubUrl);
    if (hubUrl.isEmpty()) {
        const StringList hubs = cm->getHubs(user->getCID(), hint);
        if (!hubs.empty())
            hubUrl = _q(hubs.front());
    }
    hubName = joinHubField(cm->getHubNames(user->getCID(), _tq(hubUrl)));
    ip = _q(cm->getOnlineUserIdentity(user).getIp());
}

} // namespace

qint64 ShareIndex::forceIngestListMs(const UserPtr &user, const QString &listPath,
                                     const QString &hubUrl, const QString &nick)
{
    QElapsedTimer timer;
    timer.start();
    waitWritesIdle();
    ingestListSync(user, listPath, hubUrl, nick, true);
    releaseThreadDb();
    return timer.elapsed();
}

void ShareIndex::ingestListSync(const UserPtr &user, const QString &listPath,
                                const QString &hubUrl, const QString &nick,
                                bool force)
{
    if (!user || listPath.isEmpty())
        return;
    if (PeerConnectHub::isUnreachablePeer(user))
        return;

    open();
    if (!isOpen())
        return;

    const QString cid = _q(user->getCID().toBase32());
    if (cid.isEmpty())
        return;

    if (!force && !needsListIngest(cid, listPath)) {
        setLastError(QString());
        return;
    }

    QString useHubUrl = hubUrl;
    QString useHubName;
    QString useIp;
    fillUserHubFields(user, useHubUrl, useHubName, useIp);

    if (useHubUrl.isEmpty() || useHubName.isEmpty() || useIp.isEmpty()) {
        duckdb::Connection *con = threadConn();
        if (con) {
            auto prev = ShareIndexDb::query1(
                *con,
                "SELECT ip, hub_name, hub_url FROM share_users WHERE cid = ?"
                " AND (ip != '' OR hub_name != '' OR hub_url != '')"
                " ORDER BY updated_at DESC LIMIT 1",
                ShareIndexDb::strVal(cid));
            if (prev && !prev->HasError() && prev->RowCount() > 0) {
                if (useIp.isEmpty())
                    useIp = ShareIndexDb::qstr(prev->GetValue(0, 0));
                if (useHubName.isEmpty())
                    useHubName = ShareIndexDb::qstr(prev->GetValue(1, 0));
                if (useHubUrl.isEmpty())
                    useHubUrl = ShareIndexDb::qstr(prev->GetValue(2, 0));
            }
        }
    }

    QString useNick = nick;
    if (useNick.isEmpty()) {
        WulforUtil *wu = WulforUtil::getInstance();
        if (wu)
            useNick = wu->getNicks(user->getCID(), useHubUrl);
    }

    DirectoryListing listing(HintedUser(user, _tq(useHubUrl)));
    try {
        listing.loadFile(_tq(listPath));
    } catch (const Exception &e) {
        setLastError(QString("loadFile: %1").arg(_q(e.getError())));
        return;
    } catch (const std::exception &e) {
        setLastError(QString("loadFile std: %1").arg(e.what()));
        return;
    } catch (...) {
        setLastError(QStringLiteral("loadFile unknown"));
        return;
    }

    if (isStopping())
        return;

    QList<QVariantMap> rows;
    QSet<QString> seen;
    walkListing(listing, listing.getRoot(), cid, useHubUrl, useNick, useHubName, useIp,
                seen, rows);
    if (isStopping())
        return;
    if (!writeListRows(cid, rows))
        return;
    if (isStopping())
        return;
    rememberListMeta(cid, listPath, rows.size());
    setLastError(QString());
    // Fresh list rows: attach matching queue sources while the peer connection may still be idle.
    if (user->isOnline())
        matchQueueSync(UserList{user});
}

#endif
