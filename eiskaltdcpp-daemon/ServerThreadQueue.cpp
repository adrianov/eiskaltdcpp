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
#include "ServerManager.h"
#include "ServerThread.h"

#include "dcpp/ClientManager.h"
#include "dcpp/QueueManager.h"

bool ServerThread::addInQueue(const string& sddir, const string& name, const int64_t& size, const string& tth) {
    if (name.empty() && tth.empty())
        return false;

    try
    {
        if (sddir.empty())
            QueueManager::getInstance()->add(SETTING(DOWNLOAD_DIRECTORY) + PATH_SEPARATOR_STR + name, size, TTHValue(tth));
        else
            QueueManager::getInstance()->add(sddir + PATH_SEPARATOR_STR + name, size, TTHValue(tth));
    }
    catch (const Exception& e)
    {
        if (isDebug) std::cout << "ServerThread::addInQueue->(" << e.getError() << ")"<< std::endl;
        return false;
    }

    return true;
}

bool ServerThread::setPriorityQueueItem(const string& target, const unsigned int& priority) {
    if (target.empty())
        return false;

    QueueItem::Priority p;
    switch (priority) {
        case 0:  p = QueueItem::PAUSED;  break;
        case 1:  p = QueueItem::LOWEST;  break;
        case 2:  p = QueueItem::LOW;     break;
        case 3:  p = QueueItem::NORMAL;  break;
        case 4:  p = QueueItem::HIGH;    break;
        case 5:  p = QueueItem::HIGHEST; break;
        default: p = QueueItem::PAUSED;  break;
    }

    if (target[target.length() - 1] == PATH_SEPARATOR) {
        const QueueItem::StringMap& ll = QueueManager::getInstance()->lockQueue();
        string *file;
        for (const auto& item : ll) {
            file = item.first;
            if (file->length() >= target.length() && file->substr(0, target.length()) == target)
                QueueManager::getInstance()->setPriority(*file, p);
        }
        QueueManager::getInstance()->unlockQueue();
    } else {
        QueueManager::getInstance()->setPriority(target, p);
    }
    return true;
}

void ServerThread::getItemSources(QueueItem* item, const string& separator, string& sources, unsigned int& online_tmp) {
    string nick;
    for (const auto& s : item->getSources()) {
        if (s.getUser().user->isOnline())
            ++online_tmp;
        if (!sources.empty())
            sources += separator;
        nick = Util::toString(ClientManager::getInstance()->getNicks(s.getUser().user->getCID(), s.getUser().hint));
        sources += nick;
    }
}

void ServerThread::getItemSourcesbyTarget(const string& target, const string& separator, string& sources, unsigned int& online) {
    const QueueItem::StringMap &ll = QueueManager::getInstance()->lockQueue();
    for (const auto& item : ll) {
        if (target == *(item.first)) {
            getItemSources(item.second, separator, sources, online);
        }
    }
    QueueManager::getInstance()->unlockQueue();
}

void ServerThread::getItemDescbyTarget(const string& target, StringMap& sm) {
    const QueueItem::StringMap &ll = QueueManager::getInstance()->lockQueue();
    for (const auto& item : ll) {
        if (target == *(item.first)) {
            getQueueParams(item.second,sm);
        }
    }
    QueueManager::getInstance()->unlockQueue();
}

void ServerThread::queueClear()
{
    QueueItem::StringMap &ll = QueueManager::getInstance()->lockQueue();
    ll.clear();
    QueueManager::getInstance()->unlockQueue();
}

