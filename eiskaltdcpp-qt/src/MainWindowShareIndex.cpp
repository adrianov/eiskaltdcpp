/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "MainWindow.h"
#include "ShareIndex.h"
#include "ShareBrowser.h"
#include "WulforUtil.h"
#include "Notification.h"

#include "dcpp/QueueManager.h"
#include "dcpp/QueueItem.h"
#include "dcpp/Download.h"
#include "dcpp/ClientManager.h"

using namespace dcpp;

namespace {

void queueIngest(const UserPtr &user, const QString &listPath, const QString &hubUrl)
{
    if (!user || listPath.isEmpty())
        return;

    QString nick;
    if (WulforUtil::getInstance())
        nick = WulforUtil::getInstance()->getNicks(user->getCID(), hubUrl);

    // Capture by value for the worker thread.
    const UserPtr u = user;
    const QString path = listPath;
    const QString hub = hubUrl;
    const QString n = nick;

    AsyncRunner *runner = new AsyncRunner();
    runner->setRunFunction([u, path, hub, n]() {
        ShareIndex::getInstance()->ingestList(u, path, hub, n);
    });
    QObject::connect(runner, SIGNAL(finished()), runner, SLOT(deleteLater()));
    runner->start();
}

HintedUser hintedFromQueue(QueueItem *item)
{
    if (!item)
        return HintedUser();

    if (!item->getDownloads().empty() && item->getDownloads()[0]) {
        Download *d = item->getDownloads()[0];
        return d->getHintedUser();
    }
    if (!item->getSources().empty())
        return item->getSources().front().getUser();
    return HintedUser();
}

} // namespace

void MainWindow::on(dcpp::QueueManagerListener::Finished, QueueItem *item, const std::string &dir, int64_t) noexcept
{
    if (item && item->isAnySet(QueueItem::FLAG_USER_LIST)) {
        const HintedUser hinted = hintedFromQueue(item);
        const QString listName = _q(item->getListName());
        queueIngest(hinted.user, listName, _q(hinted.hint));

        // Open ShareBrowser only when the list was requested for viewing.
        if (item->isSet(QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_USER_LIST) && hinted.user)
            emit coreOpenShare(hinted.user, listName, _q(dir));
    }

    const int qsize = QueueManager::getInstance()->lockQueue().size();
    QueueManager::getInstance()->unlockQueue();

    if (qsize == 1)
        emit notifyMessage(Notification::TRANSFER, tr("Download Queue"), tr("All downloads complete"));
}

void MainWindow::on(dcpp::QueueManagerListener::ListFromCache, const dcpp::HintedUser &user,
                    const std::string &listPath, const std::string &initialDir) noexcept
{
    queueIngest(user.user, _q(listPath), _q(user.hint));
    emit coreOpenShare(user.user, _q(listPath), _q(initialDir));
}
