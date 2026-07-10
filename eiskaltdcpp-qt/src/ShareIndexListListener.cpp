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
    if (item && item->isAnySet(QueueItem::FLAG_USER_LIST)) {
        const HintedUser hinted = hintedFromQueue(item);
        const QString listName = _q(item->getListName());
        enqueueListIngest(hinted.user, listName, _q(hinted.hint));

        if (item->isSet(QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_USER_LIST) && hinted.user)
            emit openShare(hinted.user, listName, _q(dir));
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

void ShareIndexListListener::on(QueueManagerListener::ListCached, const HintedUser &,
                                const std::string &) noexcept
{
}
