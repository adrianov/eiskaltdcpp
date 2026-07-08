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

QMenu *HubFrameMenu::buildAntispamMenu(bool showIcon){
    if (!AntiSpam::getInstance())
        return nullptr;

    QMenu *antispam_menu = new QMenu(nullptr);
    antispam_menu->setTitle(QObject::tr("AntiSpam"));

    if (showIcon) {
        antispam_menu->menuAction()->setIcon(WICON(WulforUtil::eiSPAM));
        antispam_menu->setProperty("iconVisibleInMenu", true);
    }

    antispam_menu->addAction(QObject::tr("Add to Black"))->setData(static_cast<int>(AntiSpamBlack));
    antispam_menu->addAction(QObject::tr("Add to White"))->setData(static_cast<int>(AntiSpamWhite));

    return antispam_menu;
}

HubFrameMenu::Action HubFrameMenu::resolveSelection(QAction *res, QMenu *antispam_menu,
                                                     Client *client, const QString &cid){
    if (actions.contains(res))
        return static_cast<Action>(actions.indexOf(res));

    if (chat_actions_map.contains(res))
        return chat_actions_map[res];

    if (antispam_menu && antispam_menu->actions().contains(res))
        return static_cast<Action>(res->data().toInt());

    if (!res || !res->data().canConvert(QVariant::Int))
        return None;

    const int id = res->data().toInt();

    UserCommand uc;
    if (id == -1 || !FavoriteManager::getInstance()->getUserCommand(id, uc))
        return None;

    StringMap params;
    if (!WulforUtil::getInstance()->getUserCommandParams(uc, params))
        return None;

    UserPtr user = ClientManager::getInstance()->findUser(CID(cid.toStdString()));
    if (user)
        ClientManager::getInstance()->userCommand(HintedUser(user, client->getHubUrl()), uc, params, true);

    return UserCommands;
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

    QMenu *antispam_menu = buildAntispamMenu(true);
    if (antispam_menu)
        menu->addMenu(antispam_menu);

    QAction *res = menu->exec(QCursor::pos());

    if (user_menu)
        user_menu->deleteLater();
    if (antispam_menu)
        antispam_menu->deleteLater();

    return resolveSelection(res, antispam_menu, client, cid);
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

    if (pmw){
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

    QMenu *antispam_menu = buildAntispamMenu(false);
    if (antispam_menu)
        menu->addMenu(antispam_menu);

    QAction *res = menu->exec(QCursor::pos());

    if (user_menu)
        user_menu->deleteLater();
    if (antispam_menu)
        antispam_menu->deleteLater();

    title->deleteLater();

    return resolveSelection(res, antispam_menu, client, cid);
}
