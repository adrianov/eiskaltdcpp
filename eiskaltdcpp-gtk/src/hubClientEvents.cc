/*
 * Copyright © 2004-2010 Jens Oknelid, paskharen@gmail.com
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hub ClientListener callbacks.
 */

#include "hub.hh"

#include <dcpp/ChatMessage.h>
#include <dcpp/ClientManager.h>
#include <dcpp/LogManager.h>
#include <dcpp/SettingsManager.h>
#include <dcpp/Util.h>

#include "bookentry.hh"
#include "message.hh"
#include "notify.hh"
#include "settingsmanager.hh"
#include "sound.hh"
#include "wulformanager.hh"

using namespace std;
using namespace dcpp;

void Hub::on(ClientListener::Connecting, Client *) noexcept
{
    typedef Func3<Hub, string, Msg::TypeMsg, Sound::TypeSound> F3;
    F3 *f3 = new F3(this, &Hub::addStatusMessage_gui, _("Connecting to ") + client->getHubUrl() + "...", Msg::STATUS, Sound::NONE);
    WulforManager::get()->dispatchGuiFunc(f3);
}

void Hub::on(ClientListener::Connected, Client *) noexcept
{
    typedef Func4<Hub, string, Msg::TypeMsg, Sound::TypeSound, Notify::TypeNotify> F4;
    F4 *func = new F4(this, &Hub::addStatusMessage_gui, _("Connected"), Msg::STATUS, Sound::HUB_CONNECT, Notify::HUB_CONNECT);
    WulforManager::get()->dispatchGuiFunc(func);
}

void Hub::on(ClientListener::UserUpdated, Client *, const OnlineUser &user) noexcept
{
    Identity id = user.getIdentity();

    if (!id.isHidden())
    {
        ParamMap params;
        getParams_client(params, id);
        Func1<Hub, ParamMap> *func = new Func1<Hub, ParamMap>(this, &Hub::updateUser_gui, params);
        WulforManager::get()->dispatchGuiFunc(func);
    }
}

void Hub::on(ClientListener::UsersUpdated, Client *, const OnlineUserList &list) noexcept
{
    Identity id;
    typedef Func1<Hub, ParamMap> F1;
    F1 *func;

    for (auto &it : list)
    {
        id = it->getIdentity();
        if (!id.isHidden())
        {
            ParamMap params;
            getParams_client(params, id);
            func = new F1(this, &Hub::updateUser_gui, params);
            WulforManager::get()->dispatchGuiFunc(func);
        }
    }
}

void Hub::on(ClientListener::UserRemoved, Client *, const OnlineUser &user) noexcept
{
    Func1<Hub, string> *func = new Func1<Hub, string>(this, &Hub::removeUser_gui, user.getUser()->getCID().toBase32());
    WulforManager::get()->dispatchGuiFunc(func);
}

void Hub::on(ClientListener::Redirect, Client *, const string &address) noexcept
{
    // redirect_client() crashes unless I put it into the dispatcher (why?)
    typedef Func2<Hub, string, bool> F2;
    F2 *func = new F2(this, &Hub::redirect_client, address, BOOLSETTING(AUTO_FOLLOW));
    WulforManager::get()->dispatchClientFunc(func);
}

void Hub::on(ClientListener::Failed, Client *, const string &reason) noexcept
{
    Func0<Hub> *f0 = new Func0<Hub>(this, &Hub::clearNickList_gui);
    WulforManager::get()->dispatchGuiFunc(f0);

    // Empty reason = user disconnect/reconnect: clear list, no fail status.
    if(reason.empty())
        return;

    typedef Func4<Hub, string, Msg::TypeMsg, Sound::TypeSound, Notify::TypeNotify> F4;
    F4 *f4 = new F4(this, &Hub::addStatusMessage_gui, _("Connect failed: ") + reason, Msg::SYSTEM, Sound::HUB_DISCONNECT, Notify::HUB_DISCONNECT);
    WulforManager::get()->dispatchGuiFunc(f4);
}
void Hub::on(ClientListener::GetPassword, Client *) noexcept
{
    if (!client->getPassword().empty())
        client->password(client->getPassword());
    else
    {
        Func0<Hub> *func = new Func0<Hub>(this, &Hub::getPassword_gui);
        WulforManager::get()->dispatchGuiFunc(func);
    }
}

void Hub::on(ClientListener::HubUpdated, Client *) noexcept
{
    typedef Func1<Hub, string> F1;
    string hubName;

    if (client->getHubName().empty())
        hubName = client->getAddress() + ":" + client->getPort();
    else
        hubName = client->getHubName();

    if (!client->getHubDescription().empty())
        hubName += " - " + client->getHubDescription();

    F1 *func1 = new F1(this, &BookEntry::setLabel_gui, hubName);
    WulforManager::get()->dispatchGuiFunc(func1);
}

void Hub::on(ClientListener::Message, Client*, const ChatMessage& message) noexcept
{
    if (message.text.empty() || !enableChat)
        return;

    Msg::TypeMsg typemsg;
    string cid = message.from->getIdentity().getUser()->getCID().toBase32();
    string line;

    string info=Util::formatAdditionalInfo(message.from->getIdentity().getIp(),BOOLSETTING(USE_IP),BOOLSETTING(GET_USER_COUNTRY));
    line+=info;

    if (message.thirdPerson)
        line += "* " + message.from->getIdentity().getNick() + " " +  message.text;
    else
        line += "<" + message.from->getIdentity().getNick() + "> " + message.text;

    if(message.to && message.replyTo)
    {
        //private message

        string error;
        const OnlineUser *user = (message.replyTo->getUser() == ClientManager::getInstance()->getMe())?
                    message.to : message.replyTo;

        if (message.from->getIdentity().isOp()) typemsg = Msg::OPERATOR;
        else if (message.from->getUser() == client->getMyIdentity().getUser()) typemsg = Msg::MYOWN;
        else typemsg = Msg::PRIVATE;

        if (user->getIdentity().isHub() && BOOLSETTING(IGNORE_HUB_PMS))
        {
            error = _("Ignored private message from hub");
            typedef Func3<Hub, string, Msg::TypeMsg, Sound::TypeSound> F3;
            F3 *func = new F3(this, &Hub::addStatusMessage_gui, error, Msg::STATUS, Sound::NONE);
            WulforManager::get()->dispatchGuiFunc(func);
        }
        else if (user->getIdentity().isBot() && BOOLSETTING(IGNORE_BOT_PMS))
        {
            error = _("Ignored private message from bot ") + user->getIdentity().getNick();
            typedef Func3<Hub, string, Msg::TypeMsg, Sound::TypeSound> F3;
            F3 *func = new F3(this, &Hub::addStatusMessage_gui, error, Msg::STATUS, Sound::NONE);
            WulforManager::get()->dispatchGuiFunc(func);
        }
        else
        {
            typedef Func6<Hub, Msg::TypeMsg, string, string, string, string, bool> F6;
            F6 *func = new F6(this, &Hub::addPrivateMessage_gui, typemsg, message.from->getUser()->getCID().toBase32(),
                              user->getUser()->getCID().toBase32(), client->getHubUrl(), line, true);
            WulforManager::get()->dispatchGuiFunc(func);
        }
    }
    else
    {
        // chat message

        if (message.from->getIdentity().isHub()) typemsg = Msg::STATUS;
        else if (message.from->getUser() == client->getMyIdentity().getUser()) typemsg = Msg::MYOWN;
        else typemsg = Msg::GENERAL;

        if (BOOLSETTING(FILTER_MESSAGES))
        {
            if ((message.text.find("Hub-Security") != string::npos &&
                 message.text.find("was kicked by") != string::npos) ||
                    (message.text.find("is kicking") != string::npos && message.text.find("because:") != string::npos))
            {
                typedef Func3<Hub, string, Msg::TypeMsg, Sound::TypeSound> F3;
                F3 *func = new F3(this, &Hub::addStatusMessage_gui, line, Msg::STATUS, Sound::NONE);
                WulforManager::get()->dispatchGuiFunc(func);

                return;
            }
        }

        if (BOOLSETTING(LOG_MAIN_CHAT))
        {
            StringMap params;
            params["message"] = line;
            client->getHubIdentity().getParams(params, "hub", false);
            params["hubURL"] = client->getHubUrl();
            client->getMyIdentity().getParams(params, "my", true);
            LOG(LogManager::CHAT, params);
        }

        typedef Func3<Hub, string, string, Msg::TypeMsg> F3;
        F3 *func = new F3(this, &Hub::addMessage_gui, cid, line, typemsg);
        WulforManager::get()->dispatchGuiFunc(func);

        // Set urgency hint if message contains user's nick
        if (WGETB("bold-hub") && message.from->getIdentity().getUser() != client->getMyIdentity().getUser())
        {
            if (message.text.find(client->getMyIdentity().getNick()) != string::npos)
            {
                typedef Func0<Hub> F0;
                F0 *func = new F0(this, &Hub::setUrgent_gui);
                WulforManager::get()->dispatchGuiFunc(func);
            }
        }
    }
}

void Hub::on(ClientListener::StatusMessage, Client *, const string &message, int /* flag */) noexcept
{
    if (!message.empty())
    {
        if (BOOLSETTING(FILTER_MESSAGES))
        {
            if ((message.find("Hub-Security") != string::npos && message.find("was kicked by") != string::npos) ||
                    (message.find("is kicking") != string::npos && message.find("because:") != string::npos))
            {
                typedef Func3<Hub, string, Msg::TypeMsg, Sound::TypeSound> F3;
                F3 *func = new F3(this, &Hub::addStatusMessage_gui, message, Msg::STATUS, Sound::NONE);
                WulforManager::get()->dispatchGuiFunc(func);
                return;
            }
        }

        if (BOOLSETTING(LOG_STATUS_MESSAGES))
        {
            StringMap params;
            client->getHubIdentity().getParams(params, "hub", false);
            params["hubURL"] = client->getHubUrl();
            client->getMyIdentity().getParams(params, "my", true);
            params["message"] = message;
            LOG(LogManager::STATUS, params);
        }

        typedef Func3<Hub, string, string, Msg::TypeMsg> F3;
        F3 *func = new F3(this, &Hub::addMessage_gui, "", message, Msg::STATUS);
        WulforManager::get()->dispatchGuiFunc(func);
    }
}

void Hub::on(ClientListener::NickTaken, Client *) noexcept
{
    typedef Func3<Hub, string, Msg::TypeMsg, Sound::TypeSound> F3;
    F3 *func = new F3(this, &Hub::addStatusMessage_gui, _("Nick already taken"), Msg::STATUS, Sound::NONE);
    WulforManager::get()->dispatchGuiFunc(func);
}

void Hub::on(ClientListener::SearchFlood, Client *, const string &msg) noexcept
{
    typedef Func3<Hub, string, Msg::TypeMsg, Sound::TypeSound> F3;
    F3 *func = new F3(this, &Hub::addStatusMessage_gui, _("Search spam detected from ") + msg, Msg::STATUS, Sound::NONE);
    WulforManager::get()->dispatchGuiFunc(func);
}
