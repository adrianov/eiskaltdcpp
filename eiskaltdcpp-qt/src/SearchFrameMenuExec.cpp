/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SearchFrame.h"
#include "WulforUtil.h"
#include "WulforSettings.h"
#include "DownloadToHistory.h"

#include "dcpp/UserCommand.h"

#include <QAction>
#include <QCursor>
#include <QDir>
#include <QScopedPointer>

using namespace dcpp;

SearchFrame::Menu::Action SearchFrame::Menu::exec(const QStringList &list){
    for (const auto &a : action_list)
        a->setParent(nullptr);

    qDeleteAll(down_to->actions());
    qDeleteAll(down_wh_to->actions());
    down_to->clear();
    down_wh_to->clear();

    QString aliases = QByteArray::fromBase64(WSGET(WS_DOWNLOADTO_ALIASES).toUtf8());
    QString paths = QByteArray::fromBase64(WSGET(WS_DOWNLOADTO_PATHS).toUtf8());

    QStringList a = aliases.split("\n", WULFOR_SKIP_EMPTY);
    QStringList p = paths.split("\n", WULFOR_SKIP_EMPTY);

    QStringList temp_pathes = DownloadToDirHistory::get();

    if (!temp_pathes.isEmpty()){
        for (const auto &t : temp_pathes){
            QAction *act = new QAction(WICON(WulforUtil::eiFOLDER_BLUE), QDir(t).dirName(), down_to);
            act->setToolTip(t);
            act->setData(t);
            down_to->addAction(act);

            QAction *act1 = new QAction(WICON(WulforUtil::eiFOLDER_BLUE), QDir(t).dirName(), down_to);
            act1->setToolTip(t);
            act1->setData(t);
            down_wh_to->addAction(act1);
        }

        down_to->addSeparator();
        down_wh_to->addSeparator();
    }

    if (a.size() == p.size() && !a.isEmpty()){
        for (int i = 0; i < a.size(); i++){
            QAction *act = new QAction(WICON(WulforUtil::eiFOLDER_BLUE), a.at(i), down_to);
            act->setData(p.at(i));
            down_to->addAction(act);

            QAction *act1 = new QAction(WICON(WulforUtil::eiFOLDER_BLUE), a.at(i), down_to);
            act1->setData(p.at(i));
            down_wh_to->addAction(act1);
        }

        down_to->addSeparator();
        down_wh_to->addSeparator();
    }

    QAction *browse = new QAction(WICON(WulforUtil::eiFOLDER_BLUE), tr("Browse"), down_to);
    browse->setData("");

    QAction *browse1 = new QAction(WICON(WulforUtil::eiFOLDER_BLUE), tr("Browse"), down_to);
    browse1->setData("");

    down_to->addAction(browse);
    down_wh_to->addAction(browse1);

    menu->clear();
    menu->addActions(action_list);
    menu->insertMenu(action_list.at(1), down_to);
    menu->insertMenu(action_list.at(2), down_wh_to);
    menu->insertMenu(action_list.at(5), magnet_menu);
    menu->insertMenu(action_list.at(12),black_list_menu);

    QScopedPointer<QMenu> userm(buildUserCmdMenu(list));

    if (!userm.isNull() && !userm->actions().isEmpty())
        menu->addMenu(userm.data());

    QAction *ret = menu->exec(QCursor::pos());

    if (actions.contains(ret)) {
        return actions.value(ret);
    } else if (down_to->actions().contains(ret)) {
        downToPath = ret->data().toString();
        return DownloadTo;
    } else if (down_wh_to->actions().contains(ret)) {
        downToPath = ret->data().toString();
        return DownloadWholeDirTo;
    } else if (ret && ret->data().canConvert(QVariant::Int)) {
        uc_cmd_id = ret->data().toInt();
        return UserCommands;
    } else {
        return None;
    }
}

QMenu *SearchFrame::Menu::buildUserCmdMenu(QList<QString> hub_list){
    if (hub_list.empty())
        return nullptr;

    return WulforUtil::getInstance()->buildUserCmdMenu(hub_list, UserCommand::CONTEXT_SEARCH);
}

void SearchFrame::Menu::addTempPath(const QString &path){
    QStringList temp_pathes = DownloadToDirHistory::get();
    temp_pathes.push_front(path);

    DownloadToDirHistory::put(temp_pathes);
}
