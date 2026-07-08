/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "DownloadQueue.h"

#include "DownloadQueuePrivate.h"
#include "WulforUtil.h"
#include "dcpp/QueueManager.h"
#include "dcpp/Util.h"

using namespace dcpp;

namespace {

void fillUserSourceMaps(const QueueItem *item, QMap<QString, QString> &sources, QMap<QString, QString> &badSources)
{
    for (const auto &src : item->getSources()) {
        const HintedUser &usr = src.getUser();
        if (!usr.user)
            continue;

        QString nick = WulforUtil::getInstance()->getNicks(usr.user->getCID(), _q(usr.hint));
        if (!nick.isEmpty())
            sources[nick] = _q(usr.user->getCID().toBase32());
    }

    for (const auto &src : item->getBadSources()) {
        const HintedUser &usr = src.getUser();
        if (!usr.user)
            continue;

        QString nick = WulforUtil::getInstance()->getNicks(usr.user->getCID(), _q(usr.hint));
        if (!nick.isEmpty())
            badSources[nick] = _q(usr.user->getCID().toBase32());
    }
}

void storeSourceMaps(DownloadQueuePrivate *d, const QString &target, const QueueItem *item)
{
    QMap<QString, QString> sources, badSources;
    fillUserSourceMaps(item, sources, badSources);
    d->sources[target] = sources;
    d->badSources[target] = badSources;
}

} // namespace

void DownloadQueue::syncSourceMaps(const QString &target)
{
    if (target.isEmpty())
        return;

    Q_D(DownloadQueue);
    const std::string key = target.toStdString();
    QueueManager *QM = QueueManager::getInstance();
    const QueueItem::StringMap &ll = QM->lockQueue();

    for (const auto &k : ll) {
        if (*k.first == key) {
            storeSourceMaps(d, target, k.second);
            break;
        }
    }

    QM->unlockQueue();
}

void DownloadQueue::getParams(DownloadQueue::VarMap &params, const QueueItem *item)
{
    QString nick;
    int online = 0;

    if (!item)
        return;

    params["FNAME"]  = _q(item->getTargetFileName());
    params["PATH"]   = _q(Util::getFilePath(item->getTarget()));
    params["TARGET"] = _q(item->getTarget());
    params["USERS"]  = QString("");

    QStringList user_list;

    for (const auto &src : item->getSources()) {
        const HintedUser &usr = src.getUser();
        if (!usr.user)
            continue;

        if (usr.user->isOnline())
            ++online;

        nick = WulforUtil::getInstance()->getNicks(usr.user->getCID(), _q(usr.hint));

        if (!nick.isEmpty())
            user_list.push_back(nick);
    }

    if (!user_list.isEmpty())
        params["USERS"] = user_list.join(", ");
    else
        params["USERS"] = tr("No users...");

    if (item->isWaiting())
        params["STATUS"] = tr("%1 of %2 user(s) online").arg(online).arg(item->getSources().size());
    else
        params["STATUS"] = tr("Running...");

    params["ESIZE"] = (qlonglong)item->getSize();
    params["DOWN"]  = (qlonglong)item->getDownloadedBytes();
    params["PRIO"]  = static_cast<int>(item->getPriority());

    params["ERRORS"] = QString("");

    for (const auto &src : item->getBadSources()) {
        const HintedUser &usr = src.getUser();
        if (!usr.user)
            continue;

        QString errors = params["ERRORS"].toString();
        nick = WulforUtil::getInstance()->getNicks(usr.user->getCID(), _q(usr.hint));

        if (!src.isSet(QueueItem::Source::FLAG_REMOVED)) {
            if (!errors.isEmpty())
                errors += ", ";

            errors += nick + " (";

            if (src.isSet(QueueItem::Source::FLAG_FILE_NOT_AVAILABLE))
                errors += tr("File not available");
            else if (src.isSet(QueueItem::Source::FLAG_PASSIVE))
                errors += tr("Passive user");
            else if (src.isSet(QueueItem::Source::FLAG_CRC_FAILED))
                errors += tr("Checksum mismatch");
            else if (src.isSet(QueueItem::Source::FLAG_BAD_TREE))
                errors += tr("Full tree does not match TTH root");
            else if (src.isSet(QueueItem::Source::FLAG_SLOW_SOURCE))
                errors += tr("Source too slow");
            else if (src.isSet(QueueItem::Source::FLAG_NO_TTHF))
                errors += tr("Remote client does not fully support TTH - cannot download");

            params["ERRORS"] = errors + ")";
        }
    }

    if (params["ERRORS"].toString().isEmpty())
        params["ERRORS"] = tr("No errors");

    params["ADDED"] = _q(Util::formatTime("%Y-%m-%d %H:%M", item->getAdded()));
    params["TTH"] = _q(item->getTTH().toBase32());
}
