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
#include "dcpp/ClientManager.h"
#include "dcpp/QueueManager.h"
#include "dcpp/Util.h"

using namespace dcpp;

namespace {

struct SourceInfo {
    QStringList nicks;
    int online = 0;
    QMap<QString, QString> sources;
    QMap<QString, QString> badSources;
    QString errors;
};

QString sourceNick(const HintedUser &usr)
{
    if (!usr.user)
        return QString();
    const StringList nicks = ClientManager::getInstance()->getNicks(usr.user->getCID(), usr.hint);
    return nicks.empty() ? QString() : _q(nicks[0]);
}

SourceInfo collectSources(const QueueItem *item)
{
    SourceInfo info;
    if (!item)
        return info;

    for (const auto &src : item->getSources()) {
        const HintedUser &usr = src.getUser();
        if (!usr.user)
            continue;
        if (usr.user->isOnline())
            ++info.online;
        const QString cid = _q(usr.user->getCID().toBase32());
        for (const string &n : ClientManager::getInstance()->getNicks(usr.user->getCID(), usr.hint)) {
            const QString nick = _q(n);
            if (nick.isEmpty())
                continue;
            if (!info.nicks.contains(nick))
                info.nicks.push_back(nick);
            info.sources[nick] = cid;
        }
    }

    for (const auto &src : item->getBadSources()) {
        const HintedUser &usr = src.getUser();
        if (!usr.user || src.isSet(QueueItem::Source::FLAG_REMOVED))
            continue;
        const QString nick = sourceNick(usr);
        if (nick.isEmpty())
            continue;
        info.badSources[nick] = _q(usr.user->getCID().toBase32());

        QString reason;
        if (src.isSet(QueueItem::Source::FLAG_FILE_NOT_AVAILABLE))
            reason = DownloadQueue::tr("File not available");
        else if (src.isSet(QueueItem::Source::FLAG_PASSIVE))
            reason = DownloadQueue::tr("Passive user");
        else if (src.isSet(QueueItem::Source::FLAG_UNREACHABLE))
            reason = DownloadQueue::tr("Unreachable");
        else if (src.isSet(QueueItem::Source::FLAG_CRC_FAILED))
            reason = DownloadQueue::tr("Checksum mismatch");
        else if (src.isSet(QueueItem::Source::FLAG_BAD_TREE))
            reason = DownloadQueue::tr("Full tree does not match TTH root");
        else if (src.isSet(QueueItem::Source::FLAG_SLOW_SOURCE))
            reason = DownloadQueue::tr("Source too slow");
        else if (src.isSet(QueueItem::Source::FLAG_NO_TTHF))
            reason = DownloadQueue::tr("Remote client does not fully support TTH - cannot download");
        if (reason.isEmpty())
            continue;
        if (!info.errors.isEmpty())
            info.errors += ", ";
        info.errors += nick + " (" + reason + ")";
    }
    return info;
}

void storeSourceMaps(DownloadQueuePrivate *d, const QString &target, const QueueItem *item)
{
    const SourceInfo info = collectSources(item);
    d->sources[target] = info.sources;
    d->badSources[target] = info.badSources;
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
    if (!item)
        return;

    const SourceInfo info = collectSources(item);

    params["FNAME"]  = _q(item->getTargetFileName());
    params["PATH"]   = _q(Util::getFilePath(item->getTarget()));
    params["TARGET"] = _q(item->getTarget());
    params["USERS"]  = info.nicks.isEmpty() ? tr("No users...") : info.nicks.join(", ");
    params["STATUS"] = item->isWaiting()
            ? tr("%1 of %2 user(s) online").arg(info.online).arg(item->getSources().size())
            : tr("Running...");
    params["ESIZE"]  = (qlonglong)item->getSize();
    params["DOWN"]   = (qlonglong)item->getDownloadedBytes();
    params["PRIO"]   = static_cast<int>(item->getPriority());
    params["ERRORS"] = info.errors.isEmpty() ? tr("No errors") : info.errors;
    params["ADDED"]  = _q(Util::formatTime("%Y-%m-%d %H:%M", item->getAdded()));
    params["TTH"]    = _q(item->getTTH().toBase32());
    params["SRC_ONLINE"] = info.online;
    params["SRC_TOTAL"]  = static_cast<int>(item->getSources().size());
}
