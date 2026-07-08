/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QAction>
#include <QList>
#include <QMap>
#include <QMenu>
#include <QString>

namespace dcpp { class Client; }

/**
 * Builds and executes the right-click context menus for a hub's chat log
 * and user list (and their Private Message window equivalents). A single
 * instance is shared and reference-counted across all open hub/PM windows.
 */
class HubFrameMenu
{
public:
    enum Action{
        /** Actions for userlist */
        CopyText=0,
        SearchText,
        CopyNick,
        FindInList,
        BrowseFilelist,
        MatchQueue,
        PrivateMessage,
        FavoriteAdd,
        FavoriteRem,
        GrantSlot,
        RemoveQueue,
        UserCommands,

        /** Additional actions for chat */
        ClearChat,
        FindInChat,
        DisableChat,
        SelectAllChat,
        ZoomInChat,
        ZoomOutChat,

        None,

        /** Additional actions for userlist */
        CopyComment,
        CopyIP,
        CopyShare,
        CopyTag,
        CopyEmail,

        /** Additional actions for AntiSpam */
        AntiSpamWhite,
        AntiSpamBlack
    };

    HubFrameMenu();
    ~HubFrameMenu();

    HubFrameMenu(const HubFrameMenu&) = delete;
    HubFrameMenu& operator=(const HubFrameMenu&) = delete;

    static void newInstance();
    static void deleteInstance();
    static HubFrameMenu *getInstance();

    Action execUserMenu(dcpp::Client*, const QString&);
    Action execChatMenu(dcpp::Client*, const QString&, bool pmw);

private:
    QMenu *menu;
    QList<QAction*> actions;           // actions list for menu
    QList<QAction*> pm_actions;        // actions list for menu in PMWindow
    QList<QAction*> ul_actions;        // actions list for menu in user list
    QList<QAction*> chat_actions;      // chat actions list for menu
    QList<QAction*> pm_chat_actions;   // chat actions list for menu in PMWindow
    QMap<QAction*, Action> chat_actions_map; //chat menu has separators and because of it all actions are mapped

    static HubFrameMenu *instance;
    static unsigned counter;
};
