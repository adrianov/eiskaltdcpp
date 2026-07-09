/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include "PMWindow.h"
#include "UserListModel.h"
#include "ShellCommandRunner.h"
#include "ChatSearchBar.h"

#include "dcpp/Client.h"

#include <QCompleter>
#include <QMap>
#include <QMenu>
#include <QString>
#include <QStringList>
#include <QTextCodec>
#include <QVariantMap>

class HubFramePrivate {
    typedef QMap<QString, PMWindow*> PMMap;
    typedef QVariantMap VarMap;
    typedef QList<ShellCommandRunner*> ShellList;
public:
    QMenu *arenaMenu = nullptr;

    dcpp::Client *client = nullptr;

    QTextCodec *codec = nullptr;

    quint64 total_shared = 0;
    QString hub_title;

    bool chatDisabled = false;
    bool hasMessages = false;
    bool hasHighlightMessages = false;
    bool drawLine = false;
    /** Suppress tab/tray unread until this epoch ms (connect MOTD / chat history). */
    qint64 quietUntilMs = 0;

    QStringList status_msg_history;
    QStringList out_messages;
    int out_messages_index = 0;
    bool out_messages_unsent = false;

    PMMap pm;
    ShellList shell_list;

    UserListModel *model = nullptr;
    UserListProxyModel *proxy = nullptr;

    QCompleter *completer = nullptr;

    ChatSearchBar *chatSearch = nullptr;
};
