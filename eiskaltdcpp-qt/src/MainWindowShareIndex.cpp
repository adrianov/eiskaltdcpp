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
#include "dcpp/Util.h"

#include <QDir>
#include <QFile>

using namespace dcpp;

namespace {

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

void enqueueIngest(const UserPtr &user, const QString &listPath, const QString &hubUrl)
{
    if (!user || listPath.isEmpty())
        return;

    QString nick;
    if (WulforUtil::getInstance())
        nick = WulforUtil::getInstance()->getNicks(user->getCID(), hubUrl);

    ShareIndex::getInstance()->ingestList(user, listPath, hubUrl, nick);
}

void maybeIngestUserList(const HintedUser &hinted)
{
#ifdef USE_QT_SQLITE
    if (!hinted.user)
        return;

    const QString cid = _q(hinted.user->getCID().toBase32());
    if (!ShareIndex::getInstance()->needsListIngest(cid))
        return;

    QString listPath;
    const QDir listDir(_q(Util::getListPath()));
    const QString pattern = QLatin1Char('.') + cid + QLatin1String(".xml.bz2");
    for (const QString &fn : listDir.entryList(QDir::Files)) {
        if (fn.endsWith(pattern, Qt::CaseInsensitive)) {
            listPath = listDir.absoluteFilePath(fn);
            break;
        }
    }
    if (listPath.isEmpty()) {
        const QString patternXml = QLatin1Char('.') + cid + QLatin1String(".xml");
        for (const QString &fn : listDir.entryList(QDir::Files)) {
            if (fn.endsWith(patternXml, Qt::CaseInsensitive)) {
                listPath = listDir.absoluteFilePath(fn);
                break;
            }
        }
    }
    if (listPath.isEmpty())
        return;

    enqueueIngest(hinted.user, listPath, _q(hinted.hint));
#else
    Q_UNUSED(hinted);
#endif
}

} // namespace

void MainWindow::on(dcpp::QueueManagerListener::Finished, QueueItem *item, const std::string &dir, int64_t) noexcept
{
    if (item && item->isAnySet(QueueItem::FLAG_USER_LIST)) {
        const HintedUser hinted = hintedFromQueue(item);
        const QString listName = _q(item->getListName());
        enqueueIngest(hinted.user, listName, _q(hinted.hint));

        if (item->isSet(QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_USER_LIST) && hinted.user)
            emit coreOpenShare(hinted.user, listName, _q(dir));
    } else if (item) {
        maybeIngestUserList(hintedFromQueue(item));
    }

    const int qsize = QueueManager::getInstance()->lockQueue().size();
    QueueManager::getInstance()->unlockQueue();

    if (qsize == 1)
        emit notifyMessage(Notification::TRANSFER, tr("Download Queue"), tr("All downloads complete"));
}

void MainWindow::on(dcpp::QueueManagerListener::ListFromCache, const dcpp::HintedUser &user,
                    const std::string &listPath, const std::string &initialDir) noexcept
{
    enqueueIngest(user.user, _q(listPath), _q(user.hint));
    emit coreOpenShare(user.user, _q(listPath), _q(initialDir));
}
