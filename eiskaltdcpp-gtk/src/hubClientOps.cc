/*
 * Copyright © 2004-2010 Jens Oknelid, paskharen@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hub client connect / redirect / favorite helpers.
 */

#include "hub.hh"

#include <dcpp/ClientManager.h>
#include <dcpp/ClientManagerHubGuard.h>
#include <dcpp/Exception.h>
#include <dcpp/FavoriteManager.h>
#include <dcpp/HashManager.h>
#include <dcpp/LogManager.h>
#include <dcpp/QueueManager.h>
#include <dcpp/ShareManager.h>
#include <dcpp/Text.h>
#include <dcpp/UploadManager.h>

#include "WulforUtil.hh"
#include "message.hh"
#include "settingsmanager.hh"
#include "sound.hh"
#include "wulformanager.hh"

using namespace std;
using namespace dcpp;

void Hub::connectClient_client(string address, string encoding)
{
    dcassert(client == NULL);

    if (address.substr(0, 6) == "adc://" || address.substr(0, 7) == "adcs://")
        encoding = "UTF-8";
    else if (encoding.empty() || encoding == _("Global hub default")) // latter for 1.0.3 backwards compatibility
        encoding = WGETS("default-charset");

    if (encoding == WulforUtil::ENCODING_LOCALE)
        encoding = Text::systemCharset;

    // Only pick "UTF-8" part of "UTF-8 (Unicode)".
    string::size_type i = encoding.find(' ', 0);
    if (i != string::npos)
        encoding = encoding.substr(0, i);

    client = ClientManager::getInstance()->getClient(address);
    client->setEncoding(encoding);
    client->addListener(this);
    client->connect();
    FavoriteManager::getInstance()->addListener(this);
    QueueManager::getInstance()->addListener(this);
}

void Hub::disconnect_client()
{
    if (client)
    {
        FavoriteManager::getInstance()->removeListener(this);
        QueueManager::getInstance()->removeListener(this);
        client->removeListener(this);
        client->disconnect(true);
        ClientManager::getInstance()->putClient(client);
        client = nullptr;
    }
}

void Hub::setPassword_client(string password)
{
    if (client && !password.empty())
    {
        client->setPassword(password);
        client->password(password);
    }
}

void Hub::sendMessage_client(string message, bool thirdPerson)
{
    if (client && !message.empty())
        client->hubMessage(message, thirdPerson);
}

void Hub::getFileList_client(string cid, bool match, bool full)
{
    string message;

    if (!cid.empty())
    {
        try
        {
            UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
            if (user)
            {
                const HintedUser hintedUser(user, client->getHubUrl());
                if (user == ClientManager::getInstance()->getMe())
                {
                    // Don't download file list, open locally instead
                    WulforManager::get()->getMainWindow()->openOwnList_client(true);
                }
                else if (match)
                {
                    QueueManager::getInstance()->addList(hintedUser, QueueItem::FLAG_MATCH_QUEUE);
                }
                else
                {
                    QueueManager::getInstance()->addList(hintedUser, full ? QueueItem::FLAG_CLIENT_VIEW : QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_PARTIAL_LIST);
                }
            }
            else
            {
                message = _("User not found");
            }
        }
        catch (const Exception &e)
        {
            message = e.getError();
            LogManager::getInstance()->message(message);
        }
    }

    if (!message.empty())
    {
        typedef Func3<Hub, string, Msg::TypeMsg, Sound::TypeSound> F3;
        F3 *func = new F3(this, &Hub::addStatusMessage_gui, message, Msg::SYSTEM, Sound::NONE);
        WulforManager::get()->dispatchGuiFunc(func);
    }
}

void Hub::grantSlot_client(string cid)
{
    string message = _("User not found");

    if (!cid.empty())
    {
        UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
        if (user)
        {
            const string hubUrl = client->getHubUrl();
            UploadManager::getInstance()->reserveSlot(HintedUser(user, hubUrl));
            message = _("Slot granted to ") + WulforUtil::getNicks(user, hubUrl);
        }
    }

    typedef Func3<Hub, string, Msg::TypeMsg, Sound::TypeSound> F3;
    F3 *func = new F3(this, &Hub::addStatusMessage_gui, message, Msg::STATUS, Sound::NONE);
    WulforManager::get()->dispatchGuiFunc(func);
}

void Hub::removeUserFromQueue_client(string cid)
{
    if (!cid.empty())
    {
        UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
        if (user)
            QueueManager::getInstance()->removeSource(user, QueueItem::Source::FLAG_REMOVED);
    }
}

void Hub::redirect_client(string address, bool follow)
{
    if (!address.empty())
    {
        if (ClientManagerHubGuard::hasActiveHub(address, client))
        {
            string error = _("Redirect skipped: already connected to that hub.");
            typedef Func3<Hub, string, Msg::TypeMsg, Sound::TypeSound> F3;
            F3 *f3 = new F3(this, &Hub::addStatusMessage_gui, error, Msg::STATUS, Sound::NONE);
            WulforManager::get()->dispatchGuiFunc(f3);
            return;
        }

        if (follow)
        {
            // the client is dead, long live the client!
            disconnect_client();

            Func0<Hub> *func = new Func0<Hub>(this, &Hub::clearNickList_gui);
            WulforManager::get()->dispatchGuiFunc(func);

            connectClient_client(address, encoding);
        }
    }
}

void Hub::rebuildHashData_client()
{
    HashManager::getInstance()->rebuild();
}

void Hub::refreshFileList_client()
{
    try
    {
        ShareManager::getInstance()->setDirty();
        ShareManager::getInstance()->refresh(true);
    }
    catch (const ShareException& e)
    {
    }
}

void Hub::addAsFavorite_client()
{
    typedef Func3<Hub, string, Msg::TypeMsg, Sound::TypeSound> F3;
    F3 *func;

    FavoriteHubEntry *existingHub = FavoriteManager::getInstance()->getFavoriteHubEntry(client->getHubUrl());

    if (!existingHub)
    {
        FavoriteHubEntry aEntry;
        aEntry.setServer(client->getHubUrl());
        aEntry.setName(client->getHubName());
        aEntry.setHubDescription(client->getHubDescription());
        aEntry.setConnect(false);
        aEntry.setNick(client->getMyNick());
        aEntry.setEncoding(encoding);
        FavoriteManager::getInstance()->addFavorite(aEntry);
        func = new F3(this, &Hub::addStatusMessage_gui, _("Favorite hub added"), Msg::STATUS, Sound::NONE);
        WulforManager::get()->dispatchGuiFunc(func);
    }
    else
    {
        func = new F3(this, &Hub::addStatusMessage_gui, _("Favorite hub already exists"), Msg::STATUS, Sound::NONE);
        WulforManager::get()->dispatchGuiFunc(func);
    }
}

void Hub::reconnect_client()
{
    Func0<Hub> *func = new Func0<Hub>(this, &Hub::clearNickList_gui);
    WulforManager::get()->dispatchGuiFunc(func);

    if (client)
        client->reconnect();
}
