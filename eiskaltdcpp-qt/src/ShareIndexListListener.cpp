/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ShareIndexListListener.h"
#include "ShareIndex.h"
#include "WulforUtil.h"

#include "dcpp/PeerConnectHub.h"
#include "dcpp/QueueItem.h"
#include "dcpp/Download.h"

#include <QFileInfo>

using namespace dcpp;

namespace {

HintedUser hintedFromQueue(QueueItem *item)
{
    if (!item)
        return HintedUser();
    if (!item->getDownloads().empty() && item->getDownloads()[0])
        return item->getDownloads()[0]->getHintedUser();
    if (!item->getSources().empty())
        return item->getSources().front().getUser();
    return HintedUser();
}

/** Only real file-list paths — never on ordinary download Finished. */
void enqueueListIngest(const UserPtr &user, const QString &listPath, const QString &hubUrl)
{
    if (!user || listPath.isEmpty() || !QFileInfo::exists(listPath))
        return;
    if (PeerConnectHub::isUnreachablePeer(user))
        return;
    if (!ShareIndex::getInstance())
        return;

    QString nick;
    if (WulforUtil::getInstance())
        nick = WulforUtil::getInstance()->getNicks(user->getCID(), hubUrl);
    ShareIndex::getInstance()->ingestList(user, listPath, hubUrl, nick);
}

} // namespace

ShareIndexListListener::~ShareIndexListListener()
{
    stop();
}

void ShareIndexListListener::start()
{
    QueueManager::getInstance()->addListener(this);
}

void ShareIndexListListener::stop()
{
    if (QueueManager::getInstance())
        QueueManager::getInstance()->removeListener(this);
}

void ShareIndexListListener::on(QueueManagerListener::Finished, QueueItem *item,
                                const std::string &dir, int64_t) noexcept
{
    const HintedUser hinted = hintedFromQueue(item);
    if (item && item->isAnySet(QueueItem::FLAG_USER_LIST)) {
        const QString listName = _q(item->getListName());
        enqueueListIngest(hinted.user, listName, _q(hinted.hint));

        if (item->isSet(QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_USER_LIST) && hinted.user)
            emit openShare(hinted.user, listName, _q(dir));
    } else if (hinted.user && !PeerConnectHub::isUnreachablePeer(hinted.user)
            && ShareIndex::getInstance()) {
        // Attach other queued TTHs this peer has so checkDownloads can reuse the socket.
        ShareIndex::getInstance()->matchQueue(UserList{hinted.user});
    }

    bool empty = true;
    auto &queue = QueueManager::getInstance()->lockQueue();
    for (const auto &entry : queue) {
        if (!entry.second->isFinished()) {
            empty = false;
            break;
        }
    }
    QueueManager::getInstance()->unlockQueue();
    if (empty)
        emit queueEmpty();
}

void ShareIndexListListener::on(QueueManagerListener::ListFromCache, const HintedUser &user,
                                const std::string &listPath, const std::string &initialDir) noexcept
{
    emit openShare(user.user, _q(listPath), _q(initialDir));
}

void ShareIndexListListener::on(QueueManagerListener::ListCached, const HintedUser &user,
                                const std::string &listPath) noexcept
{
    enqueueListIngest(user.user, _q(listPath), _q(user.hint));
}

void ShareIndexListListener::on(QueueManagerListener::SourceRemoved, QueueItem *item,
                                const UserPtr &user, int reason) noexcept
{
    if (!user || !ShareIndex::getInstance())
        return;

    // item may be null: unreachable fires once after the full user-source walk.
    if (reason == QueueItem::Source::FLAG_UNREACHABLE) {
        ShareIndex::getInstance()->removeUser(_q(user->getCID().toBase32()));
        return;
    }

    if (!item || reason != QueueItem::Source::FLAG_FILE_NOT_AVAILABLE)
        return;
    if (item->isSet(QueueItem::FLAG_USER_LIST))
        return;

    ShareIndex::getInstance()->removeTth(
        _q(user->getCID().toBase32()),
        _q(item->getTTH().toBase32()));
    // Peer may still have other queued files; rematch before the idle socket times out.
    if (QueueManager::getInstance()->hasDownload(user) == QueueItem::PAUSED)
        ShareIndex::getInstance()->matchQueue(UserList{user});
}
