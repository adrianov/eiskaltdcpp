/***************************************************************************
*                                                                         *
*   Copyright (C) 2009-2010  Alexandr Tkachev <tka4ev@gmail.com>          *
*   Copyright (C) 2020 Boris Pek <tehnick-8@yandex.ru>                    *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "stdafx.h"
#include "utility.h"
#include "ServerThread.h"

#include "dcpp/ClientManager.h"
#include "dcpp/QueueManager.h"

void ServerThread::getQueueParams(QueueItem* item, StringMap& params) {
    string nick;
    unsigned int online = 0;

    params["Filename"] = item->getTargetFileName();
    params["Path"] = Util::getFilePath(item->getTarget());
    params["Target"] = item->getTarget();

    params["Users"] = "";

    getItemSources(item, ", ", params["Users"], online);

    if (params["Users"].empty())
        params["Users"] = _("No users");

    // Status
    if (item->isWaiting())
        params["Status"] = Util::toString(online) + _(" of ") + Util::toString(item->getSources().size()) + _(" user(s) online");
    else
        params["Status"] = _("Running...");

    // Size
    params["Size Sort"] = Util::toString(item->getSize());
    if (item->getSize() < 0) {
        params["Size"] = _("Unknown");
        params["Exact Size"] = _("Unknown");
    } else {
        params["Size"] = Util::formatBytes(item->getSize());
        params["Exact Size"] = Util::formatExactSize(item->getSize());
    }

    // Downloaded
    params["Downloaded Sort"] = Util::toString(item->getDownloadedBytes());
    if (item->getSize() > 0) {
        double percent = (double)item->getDownloadedBytes() * 100.0 / (double)item->getSize();
        params["Downloaded"] = Util::formatBytes(item->getDownloadedBytes()) + " (" + Util::toString(percent) + "%)";
    } else {
        params["Downloaded"] = _("0 B (0.00%)");
    }

    // Priority
    switch (item->getPriority()) {
        case QueueItem::PAUSED: params["Priority"] = _("Paused"); break;
        case QueueItem::LOWEST: params["Priority"] = _("Lowest"); break;
        case QueueItem::LOW: params["Priority"] = _("Low"); break;
        case QueueItem::HIGH: params["Priority"] = _("High"); break;
        case QueueItem::HIGHEST: params["Priority"] = _("Highest"); break;
        default: params["Priority"] = _("Normal"); break;
    }

    // Error
    params["Errors"] = "";
    for (const auto& s : item->getBadSources()) {
        nick = Util::toString(ClientManager::getInstance()->getNicks(s.getUser().user->getCID(), s.getUser().hint));

        if (!s.isSet(QueueItem::Source::FLAG_REMOVED)) {
            if (params["Errors"].size() > 0)
                params["Errors"] += ", ";
            params["Errors"] += nick + " (";

            if (s.isSet(QueueItem::Source::FLAG_FILE_NOT_AVAILABLE))
                params["Errors"] += _("File not available");
            else if (s.isSet(QueueItem::Source::FLAG_PASSIVE))
                params["Errors"] += _("Passive user");
            else if (s.isSet(QueueItem::Source::FLAG_CRC_FAILED))
                params["Errors"] += _("CRC32 inconsistency (SFV-Check)");
            else if (s.isSet(QueueItem::Source::FLAG_BAD_TREE))
                params["Errors"] += _("Full tree does not match TTH root");
            else if (s.isSet(QueueItem::Source::FLAG_SLOW_SOURCE))
                params["Errors"] += _("Source too slow");
            else if (s.isSet(QueueItem::Source::FLAG_NO_TTHF))
                params["Errors"] += _("Remote client does not fully support TTH - cannot download");

            params["Errors"] += ")";
        }
    }
    if (params["Errors"].empty())
        params["Errors"] = _("No errors");

    // Added
    params["Added"] = Util::formatTime("%Y-%m-%d %H:%M", item->getAdded());

    // TTH
    params["TTH"] = item->getTTH().toBase32();
}

void ServerThread::listQueueTargets(string& listqueue, const string& sseparator) {
    string separator;
    if (sseparator.empty())
        separator = "\n";
    else
        separator = sseparator;
    const QueueItem::StringMap &ll = QueueManager::getInstance()->lockQueue();

    for (const auto& item : ll) {
        listqueue += *(item.first);
        listqueue += separator;
    }
    QueueManager::getInstance()->unlockQueue();
}

void ServerThread::listQueue(unordered_map<string,StringMap>& listqueue) {
    const QueueItem::StringMap &ll = QueueManager::getInstance()->lockQueue();
    for (const auto& item : ll) {
        StringMap sm;
        getQueueParams(item.second,sm);
        listqueue[*(item.first)] = sm;
    }
    QueueManager::getInstance()->unlockQueue();
}

bool ServerThread::moveQueueItem(const string& source, const string& target) {
    if (!source.empty() && !target.empty()) {
        if (target[target.length() - 1] == PATH_SEPARATOR) {
            // Can't modify QueueItem::StringMap in the loop, so we have to queue them.
            vector<string> targets;
            string *file;
            const QueueItem::StringMap &ll = QueueManager::getInstance()->lockQueue();

            for (const auto& item : ll) {
                file = item.first;
                if (file->length() >= source.length() && file->substr(0, source.length()) == source)
                    targets.push_back(*file);
            }
            QueueManager::getInstance()->unlockQueue();

            for (const auto& item : targets) {
                QueueManager::getInstance()->move(item, target + item.substr(source.length()));
            }
        } else {
            QueueManager::getInstance()->move(source, target);
        }
        return true;
    }
    return false;
}

bool ServerThread::removeQueueItem(const string& target) {
    if (!target.empty()) {
        if (target[target.length() - 1] == PATH_SEPARATOR) {
            string *file;
            vector<string> targets;
            const QueueItem::StringMap &ll = QueueManager::getInstance()->lockQueue();

            for (const auto& item : ll) {
                file = item.first;
                if (file->length() >= target.length() && file->substr(0, target.length()) == target)
                    targets.push_back(*file);
            }
            QueueManager::getInstance()->unlockQueue();

            for (const auto& item : targets) {
                QueueManager::getInstance()->remove(item);
            }
        } else {
            QueueManager::getInstance()->remove(target);
        }
        return true;
    }
    return false;
}

