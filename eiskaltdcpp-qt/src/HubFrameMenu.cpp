/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "HubFrameMenu.h"

#include "Antispam.h"
#include "WulforUtil.h"

#include "dcpp/CID.h"
#include "dcpp/Client.h"
#include "dcpp/ClientManager.h"
#include "dcpp/FavoriteManager.h"
#include "dcpp/UserCommand.h"

#include <QCursor>
#include <QFont>
#include <QVariant>

using namespace dcpp;

HubFrameMenu *HubFrameMenu::instance = nullptr;
unsigned HubFrameMenu::counter = 0;

void HubFrameMenu::newInstance(){
    counter++;

    if (instance)
        return;

    instance = new HubFrameMenu();
}

void HubFrameMenu::deleteInstance(){
    counter--;

    if (counter)
        return;

    delete instance;

    instance = nullptr;
}

HubFrameMenu *HubFrameMenu::getInstance(){
    return instance;
}

HubFrameMenu::HubFrameMenu() : menu(new QMenu(nullptr))
{
    WulforUtil *WU = WulforUtil::getInstance();

    // Userlist actions
    QAction *copy_text   = new QAction(WU->getPixmap(WulforUtil::eiEDITCOPY), QObject::tr("Copy"), nullptr);
    QAction *search_text = new QAction(WU->getPixmap(WulforUtil::eiFIND), QObject::tr("Search text"), nullptr);
    QAction *copy_nick   = new QAction(WU->getPixmap(WulforUtil::eiEDITCOPY), QObject::tr("Copy nick"), nullptr);
    QAction *find        = new QAction(WU->getPixmap(WulforUtil::eiFIND), QObject::tr("Show in list"), nullptr);
    QAction *browse      = new QAction(WU->getPixmap(WulforUtil::eiFOLDER_BLUE), QObject::tr("Browse files"), nullptr);
    QAction *match_queue = new QAction(WU->getPixmap(WulforUtil::eiDOWN), QObject::tr("Match Queue"), nullptr);
    QAction *private_msg = new QAction(WU->getPixmap(WulforUtil::eiMESSAGE), QObject::tr("Private Message"), nullptr);
    QAction *fav_add     = new QAction(WU->getPixmap(WulforUtil::eiFAVADD), QObject::tr("Add to Favorites"), nullptr);
    QAction *fav_del     = new QAction(WU->getPixmap(WulforUtil::eiFAVREM), QObject::tr("Remove from Favorites"), nullptr);
    QAction *grant_slot  = new QAction(WU->getPixmap(WulforUtil::eiEDITADD), QObject::tr("Grant slot"), nullptr);
    QAction *rem_queue   = new QAction(WU->getPixmap(WulforUtil::eiEDITDELETE), QObject::tr("Remove from Queue"), nullptr);

    // Chat actions
    QAction *sep1        = new QAction(nullptr);
    QAction *clear_chat  = new QAction(WU->getPixmap(WulforUtil::eiCLEAR), QObject::tr("Clear chat"), nullptr);
    QAction *find_in_chat= new QAction(WU->getPixmap(WulforUtil::eiFIND), QObject::tr("Find in chat"), nullptr);
    QAction *dis_chat    = new QAction(WU->getPixmap(WulforUtil::eiFILECLOSE), QObject::tr("Disable/Enable chat"), nullptr);
    QAction *sep2        = new QAction(nullptr);
    QAction *select_all  = new QAction(QObject::tr("Select all"), nullptr);
    QAction *sep3        = new QAction(nullptr);
    QAction *zoom_in     = new QAction(WU->getPixmap(WulforUtil::eiZOOM_IN), QObject::tr("Zoom In"), nullptr);
    QAction *zoom_out    = new QAction(WU->getPixmap(WulforUtil::eiZOOM_OUT), QObject::tr("Zoom Out"), nullptr);

    // submenu copy_data for user list
    QAction *copy_data_nick  = new QAction(QObject::tr("Nick"), nullptr);
    QAction *copy_data_cmnt  = new QAction(QObject::tr("Comment"), nullptr);
    QAction *copy_data_ip    = new QAction(QObject::tr("IP"), nullptr);
    QAction *copy_data_share = new QAction(QObject::tr("Share"), nullptr);
    QAction *copy_data_tag   = new QAction(QObject::tr("Tag"), nullptr);
    QAction *copy_data_email = new QAction(QObject::tr("E-mail"), nullptr);
    QAction *sep4            = new QAction(nullptr);
    QAction *copy_data_all   = new QAction(QObject::tr("All"), nullptr);

    QMenu *menuCopyData = new QMenu(nullptr);
    menuCopyData->addActions(QList<QAction*>() << copy_data_nick << copy_data_cmnt << copy_data_ip << copy_data_share << copy_data_tag << copy_data_email << sep4 << copy_data_all);

    QAction *copy_data   = new QAction(WU->getPixmap(WulforUtil::eiEDITCOPY), QObject::tr("Copy data"), nullptr);
    copy_data->setMenu(menuCopyData);
    // end submenu

    sep1->setSeparator(true), sep2->setSeparator(true), sep3->setSeparator(true), sep4->setSeparator(true);

    actions << copy_text
            << search_text
            << copy_nick
            << find
            << browse
            << match_queue
            << private_msg
            << fav_add
            << fav_del
            << grant_slot
            << rem_queue;

    pm_actions << copy_text
            << copy_nick
            << browse
            << match_queue
            << private_msg
            << fav_add
            << fav_del
            << grant_slot
            << rem_queue;

    ul_actions << browse
            << private_msg
            << fav_add
            << fav_del
            << grant_slot
            << copy_data
            << match_queue
            << rem_queue;

    chat_actions << sep1
                 << clear_chat
                 << find_in_chat
                 << dis_chat
                 << sep2
                 << select_all
                 << sep3
                 << zoom_in
                 << zoom_out;

    pm_chat_actions << sep1
                 << clear_chat
                 << sep2
                 << select_all
                 << sep3
                 << zoom_in
                 << zoom_out;

    chat_actions_map.insert(copy_text, CopyText);
    chat_actions_map.insert(search_text, SearchText);
    chat_actions_map.insert(copy_nick, CopyNick);
    chat_actions_map.insert(clear_chat, ClearChat);
    chat_actions_map.insert(find_in_chat, FindInChat);
    chat_actions_map.insert(dis_chat, DisableChat);
    chat_actions_map.insert(select_all, SelectAllChat);
    chat_actions_map.insert(zoom_in, ZoomInChat);
    chat_actions_map.insert(zoom_out, ZoomOutChat);

    chat_actions_map.insert(copy_data_nick,  CopyNick);
    chat_actions_map.insert(copy_data_cmnt,  CopyComment);
    chat_actions_map.insert(copy_data_ip,    CopyIP);
    chat_actions_map.insert(copy_data_share, CopyShare);
    chat_actions_map.insert(copy_data_tag,   CopyTag);
    chat_actions_map.insert(copy_data_email, CopyEmail);
    chat_actions_map.insert(copy_data_all,   CopyText);
}

HubFrameMenu::~HubFrameMenu(){
    delete menu;
    menu = nullptr;

    qDeleteAll(chat_actions);
    qDeleteAll(actions);
    chat_actions.clear();
    actions.clear();
}

HubFrameMenu::Action HubFrameMenu::execUserMenu(Client *client, const QString &cid){
   if (!client)
        return None;

    menu->clear();
    menu->setProperty("iconVisibleInMenu", true);

    menu->setTitle(WulforUtil::getInstance()->getNickViaOnlineUser(cid, _q(client->getHubUrl())));

    if (menu->title().isEmpty())
        menu->setTitle(QObject::tr("[User went offline]"));

    menu->addActions(ul_actions);

    QMenu *user_menu = nullptr;

    if (!cid.isEmpty()){
        user_menu = WulforUtil::getInstance()->buildUserCmdMenu(client->getHubUrl(), UserCommand::CONTEXT_USER);

        if (user_menu && !user_menu->actions().empty())
            menu->addMenu(user_menu);
    }

    QMenu *antispam_menu = nullptr;

    if (AntiSpam::getInstance()){
        antispam_menu = new QMenu(nullptr);
        antispam_menu->setTitle(QObject::tr("AntiSpam"));
        antispam_menu->menuAction()->setIcon(WICON(WulforUtil::eiSPAM));
        antispam_menu->setProperty("iconVisibleInMenu", true);

        antispam_menu->addAction(QObject::tr("Add to Black"))->setData(static_cast<int>(AntiSpamBlack));
        antispam_menu->addAction(QObject::tr("Add to White"))->setData(static_cast<int>(AntiSpamWhite));

        menu->addMenu(antispam_menu);
    }

    QAction *res = menu->exec(QCursor::pos());

    if (user_menu)
        user_menu->deleteLater();
    if (antispam_menu)
        antispam_menu->deleteLater();

    if (actions.contains(res))
        return static_cast<Action>(actions.indexOf(res));
    else if (chat_actions_map.contains(res))
        return chat_actions_map[res];
    else if (antispam_menu && antispam_menu->actions().contains(res))
        return static_cast<HubFrameMenu::Action>(res->data().toInt());
    else if (res && res->data().canConvert(QVariant::Int)){//User command
        int id = res->data().toInt();

        UserCommand uc;
        if (id == -1 || !FavoriteManager::getInstance()->getUserCommand(id, uc))
            return None;

        StringMap params;

        if (WulforUtil::getInstance()->getUserCommandParams(uc, params)) {
            UserPtr user = ClientManager::getInstance()->findUser(CID(cid.toStdString()));

            if (user)
                ClientManager::getInstance()->userCommand(HintedUser(user, client->getHubUrl()), uc, params, true);

            return UserCommands;
        }
        else
            return None;
    }
    else
        return None;
}

HubFrameMenu::Action HubFrameMenu::execChatMenu(Client *client, const QString &cid, bool pmw){
    if (!client)
        return None;

    menu->clear();
    QAction *title = new QAction(WulforUtil::getInstance()->getNickViaOnlineUser(cid, _q(client->getHubUrl())), menu);
    QFont f;
    f.setBold(true);
    title->setFont(f);
    title->setEnabled(false);

    if (title->text().isEmpty())
        title->setText(QObject::tr("[User went offline]"));

    menu->addAction(title);

    if(pmw){
        menu->addActions(pm_actions);
        menu->addActions(pm_chat_actions);
    }
    else{
        menu->addActions(actions);
        menu->addActions(chat_actions);
    }

    QMenu *user_menu = nullptr;

    if (!cid.isEmpty() && !pmw){
        user_menu = WulforUtil::getInstance()->buildUserCmdMenu(client->getHubUrl(), UserCommand::CONTEXT_HUB);

        if (user_menu && !user_menu->actions().isEmpty())
            menu->addMenu(user_menu);
    }

    QMenu *antispam_menu = nullptr;

    if (AntiSpam::getInstance()){
        antispam_menu = new QMenu(nullptr);
        antispam_menu->setTitle(QObject::tr("AntiSpam"));

        antispam_menu->addAction(QObject::tr("Add to Black"))->setData(static_cast<int>(AntiSpamBlack));
        antispam_menu->addAction(QObject::tr("Add to White"))->setData(static_cast<int>(AntiSpamWhite));

        menu->addMenu(antispam_menu);
    }

    QAction *res = menu->exec(QCursor::pos());

    if (user_menu)
        user_menu->deleteLater();

    title->deleteLater();

    if (actions.contains(res))
        return static_cast<Action>(actions.indexOf(res));
    else if (chat_actions_map.contains(res))
        return chat_actions_map[res];
    else if (antispam_menu && antispam_menu->actions().contains(res))
        return static_cast<HubFrameMenu::Action>(res->data().toInt());
    else if (res && res->data().canConvert(QVariant::Int)){//User command
        int id = res->data().toInt();

        UserCommand uc;
        if (id == -1 || !FavoriteManager::getInstance()->getUserCommand(id, uc))
            return None;

        StringMap params;

        if (WulforUtil::getInstance()->getUserCommandParams(uc, params)){
            UserPtr user = ClientManager::getInstance()->findUser(CID(cid.toStdString()));

            if (user)
                ClientManager::getInstance()->userCommand(HintedUser(user, client->getHubUrl()), uc, params, true);

            return UserCommands;
        }
        else
            return None;
    }
    else
        return None;
}
