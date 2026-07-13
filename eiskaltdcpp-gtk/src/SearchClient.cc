/*
 * Copyright © 2004-2010 Jens Oknelid, paskharen@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Client-thread helpers for Search (queue / lists / favorites).
 */

#include "search.hh"

#include <dcpp/FavoriteManager.h>
#include <dcpp/QueueManager.h>
#include <dcpp/UploadManager.h>
#include "WulforUtil.hh"

using namespace std;
using namespace dcpp;

void Search::download_client(string target, string cid, string filename, int64_t size, string tth, string hubUrl)
{
    try
    {
        UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
        if (!user)
            return;

        if (!tth.empty())
        {
            string subdir = Util::getFileName(filename);
            QueueManager::getInstance()->add(target + subdir, size, TTHValue(tth), HintedUser(user, hubUrl));
        }
        else
        {
            string dir = WulforUtil::windowsSeparator(filename);
            QueueManager::getInstance()->addDirectory(dir, HintedUser(user, hubUrl), target);
        }
    }
    catch (const Exception&)
    {
    }
}

void Search::downloadDir_client(string target, string cid, string filename, string hubUrl)
{
    try
    {
        string dir;

        if (filename[filename.length() - 1] != PATH_SEPARATOR)
            dir = WulforUtil::windowsSeparator(Util::getFilePath(filename));
        else
            dir = WulforUtil::windowsSeparator(filename);

        UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
        if (user)
            QueueManager::getInstance()->addDirectory(dir, HintedUser(user, hubUrl), target);
    }
    catch (const Exception&)
    {
    }
}

void Search::addSource_client(string source, string cid, int64_t size, string tth, string hubUrl)
{
    try
    {
        UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
        if (!tth.empty() && user)
            QueueManager::getInstance()->add(source, size, TTHValue(tth), HintedUser(user, hubUrl));
    }
    catch (const Exception&)
    {
    }
}

void Search::getFileList_client(string cid, string dir, bool match, string hubUrl, bool full)
{
    if (cid.empty())
        return;

    try
    {
        UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
        if (!user)
            return;

        int flags = 0;
        if (match)
            flags = QueueItem::FLAG_MATCH_QUEUE;
        else
            flags = QueueItem::FLAG_CLIENT_VIEW;

        if (!full)
            flags = QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_PARTIAL_LIST;

        QueueManager::getInstance()->addList(HintedUser(user, hubUrl), flags, dir);
    }
    catch (const Exception&)
    {
    }
}

void Search::grantSlot_client(string cid, string hubUrl)
{
    if (cid.empty())
        return;
    UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
    if (user)
        UploadManager::getInstance()->reserveSlot(HintedUser(user, hubUrl));
}

void Search::addFavUser_client(string cid)
{
    if (cid.empty())
        return;
    UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
    if (user)
        FavoriteManager::getInstance()->addFavoriteUser(user);
}

void Search::removeSource_client(string cid)
{
    if (cid.empty())
        return;
    UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
    if (user)
        QueueManager::getInstance()->removeSource(user, QueueItem::Source::FLAG_REMOVED);
}
