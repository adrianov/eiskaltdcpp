/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "ShareBrowser.h"
#include "WulforUtil.h"
#include "WulforSettings.h"
#include "MainWindow.h"
#include "DownloadToHistory.h"

#include "dcpp/ClientManager.h"

#include <QAction>
#include <QCursor>
#include <QDir>

using namespace dcpp;

ShareBrowser::Menu::Menu() : menu(new QMenu(nullptr))
{
    WulforUtil *WU = WulforUtil::getInstance();

    rest_menu = new QMenu(tr("Restrictions"));
    QMenu *magnet_menu = new QMenu(tr("Magnet"), MainWindow::getInstance());

    QAction *down    = new QAction(tr("Download"), menu);
    down->setIcon(WU->getPixmap(WulforUtil::eiDOWNLOAD));
    down_to = new QMenu(tr("Download to..."));
    down_to->setIcon(WU->getPixmap(WulforUtil::eiDOWNLOAD_AS));
    QAction *sep     = new QAction(menu);
    QAction *alter   = new QAction(tr("Search for alternates"), menu);
    alter->setIcon(WU->getPixmap(WulforUtil::eiFILEFIND));
    QAction *copy_name = new QAction(tr("Copy file name"), menu);
    copy_name->setIcon(WU->getPixmap(WulforUtil::eiEDITCOPY));
    QAction *magnet  = new QAction(tr("Copy magnet"), menu);
    magnet->setIcon(WU->getPixmap(WulforUtil::eiEDITCOPY));
    QAction *magnet_web  = new QAction(tr("Copy web-magnet"), menu);
    magnet_web->setIcon(WU->getPixmap(WulforUtil::eiEDITCOPY));
    QAction *magnet_info  = new QAction(tr("Properties of magnet"), menu);
    magnet_info->setIcon(WU->getPixmap(WulforUtil::eiDOWNLOAD));
    QAction *sep1    = new QAction(menu);
    QAction *add_to_fav = new QAction(tr("Add to favorites"), menu);
    add_to_fav->setIcon(WU->getPixmap(WulforUtil::eiBOOKMARK_ADD));
    QAction *set_rest = new QAction(tr("Add restriction"), rest_menu);
    QAction *rem_rest = new QAction(tr("Remove restriction"), rest_menu);
    open_file = new QAction(WU->getPixmap(WulforUtil::eiFILETYPE_UNKNOWN), tr("Open file"), menu);
    open_url = new QAction(WU->getPixmap(WulforUtil::eiFOLDER_BLUE), tr("Open directory"), menu);
    delete_file = new QAction(WU->getPixmap(WulforUtil::eiEDITDELETE), tr("Delete File"), menu);
    QAction *sep2    = new QAction(menu);
    QAction *sep3    = new QAction(menu);
    QAction *sep4    = new QAction(menu);

    actions.insert(down, Download);
    actions.insert(alter, Alternates);
    actions.insert(copy_name, CopyFileName);
    actions.insert(magnet, Magnet);
    actions.insert(magnet_web, MagnetWeb);
    actions.insert(magnet_info, MagnetInfo);
    actions.insert(add_to_fav, AddToFav);
    actions.insert(set_rest, AddRestrinction);
    actions.insert(rem_rest, RemoveRestriction);
    actions.insert(open_file, OpenFile);
    actions.insert(open_url, OpenUrl);
    actions.insert(delete_file, DeleteFile);

    magnet_menu->addActions(QList<QAction*>()
                    << magnet << magnet_web << sep3 << magnet_info);

    sep->setSeparator(true);
    sep1->setSeparator(true);
    sep2->setSeparator(true);
    sep3->setSeparator(true);
    sep4->setSeparator(true);

    menu->addActions(QList<QAction*>() << down << sep << alter << copy_name);
    menu->addMenu(magnet_menu);
    menu->addActions(QList<QAction*>() << sep1 << add_to_fav << sep2);
    rest_menu->addActions(QList<QAction*>() << set_rest << rem_rest);
    menu->insertMenu(sep, down_to);
    menu->addMenu(rest_menu);
    menu->addActions(QList<QAction*>() << open_file << open_url << sep4 << delete_file);
}

ShareBrowser::Menu::~Menu(){
    delete menu;
    delete rest_menu;
    delete down_to;

    menu = nullptr;
    rest_menu = nullptr;
    down_to = nullptr;
}

ShareBrowser::Menu::Action ShareBrowser::Menu::exec(const dcpp::UserPtr &user, bool treePane,
                                                    bool hasDeletable){
    qDeleteAll(down_to->actions());
    down_to->clear();

    const QPixmap &dir_px = WICON(WulforUtil::eiFOLDER_BLUE);
    QString aliases, paths;

    aliases = QByteArray::fromBase64(WSGET(WS_DOWNLOADTO_ALIASES).toUtf8());
    paths   = QByteArray::fromBase64(WSGET(WS_DOWNLOADTO_PATHS).toUtf8());

    QStringList a = aliases.split("\n", WULFOR_SKIP_EMPTY);
    QStringList p = paths.split("\n", WULFOR_SKIP_EMPTY);

    QStringList temp_pathes = DownloadToDirHistory::get();

    if (!temp_pathes.isEmpty()){
        for (const auto &t : temp_pathes){
            QAction *act = new QAction(WICON(WulforUtil::eiFOLDER_BLUE), QDir(t).dirName(), down_to);
            act->setToolTip(t);
            act->setData(t);

            down_to->addAction(act);
        }

        down_to->addSeparator();
    }

    if (a.size() == p.size() && !a.isEmpty()){
        for (int i = 0; i < a.size(); i++){
            QAction *act = new QAction(a.at(i), down_to);
            act->setData(p.at(i));
            act->setIcon(dir_px);

            down_to->addAction(act);
        }

        down_to->addSeparator();
    }

    QAction *browse = new QAction(WICON(WulforUtil::eiFOLDER_BLUE), tr("Browse"), down_to);
    browse->setIcon(dir_px);
    browse->setData("");

    down_to->addAction(browse);

    const bool own = (user == ClientManager::getInstance()->getMe());
    rest_menu->setEnabled(own && treePane);
    open_file->setEnabled(own);
    open_url->setEnabled(own);
    delete_file->setEnabled(own && hasDeletable);

    QAction *ret = menu->exec(QCursor::pos());

    if (actions.contains(ret))
        return actions.value(ret);
    else if (down_to->actions().contains(ret)){
        target = ret->data().toString();

        return DownloadTo;
    }

    return None;
}
