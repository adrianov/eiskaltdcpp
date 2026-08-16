/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "search/SearchResultsMenu.h"
#include "downloadto/DownloadToHistory.h"
#include "downloadto/DownloadToMenu.h"
#include "WulforUtil.h"

#include "dcpp/UserCommand.h"

#include <QAction>
#include <QCursor>
#include <QScopedPointer>

using namespace dcpp;

SearchResultsMenu::Action SearchResultsMenu::exec(const QStringList &list, bool canOpenLocal)
{
    for (const auto &a : action_list)
        a->setParent(nullptr);

    down_to->refill();
    down_wh_to->refill();

    // Open file / Open directory are action_list[2] and [3]
    action_list.at(2)->setEnabled(canOpenLocal);
    action_list.at(3)->setEnabled(canOpenLocal);

    menu->clear();
    menu->addActions(action_list);
    menu->insertMenu(action_list.at(1), down_to);
    menu->insertMenu(action_list.at(2), down_wh_to);
    menu->insertMenu(action_list.at(8), magnet_menu);
    menu->insertMenu(action_list.at(15), black_list_menu);

    QScopedPointer<QMenu> userm(buildUserCmdMenu(list));

    if (!userm.isNull() && !userm->actions().isEmpty())
        menu->addMenu(userm.data());

    QAction *ret = menu->exec(QCursor::pos());

    if (actions.contains(ret)) {
        return actions.value(ret);
    } else if (down_to->takeTarget(ret, downToPath)) {
        return DownloadTo;
    } else if (down_wh_to->takeTarget(ret, downToPath)) {
        return DownloadWholeDirTo;
    } else if (ret && ret->data().canConvert<int>()) {
        uc_cmd_id = ret->data().toInt();
        return UserCommands;
    } else {
        return None;
    }
}

QMenu *SearchResultsMenu::buildUserCmdMenu(QList<QString> hub_list)
{
    if (hub_list.empty())
        return nullptr;

    return WulforUtil::getInstance()->buildUserCmdMenu(hub_list, UserCommand::CONTEXT_SEARCH);
}

void SearchResultsMenu::addTempPath(const QString &path)
{
    QStringList temp_pathes = DownloadToDirHistory::get();
    temp_pathes.push_front(path);

    DownloadToDirHistory::put(temp_pathes);
}
