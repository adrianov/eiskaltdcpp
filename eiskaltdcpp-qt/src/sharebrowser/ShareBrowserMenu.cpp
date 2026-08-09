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

#include "sharebrowser/ShareBrowserMenu.h"
#include "WulforUtil.h"
#include "WulforSettings.h"
#include "DownloadToHistory.h"

#include "dcpp/ClientManager.h"

#include <QAction>
#include <QCoreApplication>
#include <QCursor>
#include <QDir>

using namespace dcpp;

namespace {

QString sbTr(const char *s)
{
    return QCoreApplication::translate("ShareBrowser", s);
}

/** Reuse SearchFrame catalog entries for the shared whole-dir labels. */
QString wholeDirTr(const char *s)
{
    return QCoreApplication::translate("SearchFrame", s);
}

void fillDownloadTo(QMenu *menu, const QStringList &tempPaths, const QStringList &aliases,
                    const QStringList &paths, const QPixmap &dirPx)
{
    if (!tempPaths.isEmpty()) {
        for (const auto &t : tempPaths) {
            QAction *act = new QAction(dirPx, QDir(t).dirName(), menu);
            act->setToolTip(t);
            act->setData(t);
            menu->addAction(act);
        }
        menu->addSeparator();
    }

    if (aliases.size() == paths.size() && !aliases.isEmpty()) {
        for (int i = 0; i < aliases.size(); i++) {
            QAction *act = new QAction(aliases.at(i), menu);
            act->setData(paths.at(i));
            act->setIcon(dirPx);
            menu->addAction(act);
        }
        menu->addSeparator();
    }

    QAction *browse = new QAction(dirPx, sbTr("Browse"), menu);
    browse->setData("");
    menu->addAction(browse);
}

} // namespace

ShareBrowserMenu::ShareBrowserMenu() : menu(new QMenu(nullptr))
{
    WulforUtil *WU = WulforUtil::getInstance();

    rest_menu = new QMenu(sbTr("Restrictions"));
    QMenu *magnet_menu = new QMenu(sbTr("Magnet"), menu);

    QAction *down    = new QAction(sbTr("Download"), menu);
    down->setIcon(WU->getPixmap(AppIcons::eiDOWNLOAD));
    down_to = new QMenu(sbTr("Download to..."));
    down_to->setIcon(WU->getPixmap(AppIcons::eiDOWNLOAD_AS));
    QAction *down_wh = new QAction(wholeDirTr("Download Whole Directory"), menu);
    down_wh->setIcon(WU->getPixmap(AppIcons::eiDOWNLOAD));
    down_wh_to = new QMenu(wholeDirTr("Download Whole Directory to..."));
    down_wh_to->setIcon(WU->getPixmap(AppIcons::eiDOWNLOAD_AS));
    QAction *sep     = new QAction(menu);
    QAction *alter   = new QAction(sbTr("Search for alternates"), menu);
    alter->setIcon(WU->getPixmap(AppIcons::eiFILEFIND));
    QAction *copy_name = new QAction(sbTr("Copy file name"), menu);
    copy_name->setIcon(WU->getPixmap(AppIcons::eiEDITCOPY));
    QAction *magnet  = new QAction(sbTr("Copy magnet"), menu);
    magnet->setIcon(WU->getPixmap(AppIcons::eiEDITCOPY));
    QAction *magnet_web  = new QAction(sbTr("Copy web-magnet"), menu);
    magnet_web->setIcon(WU->getPixmap(AppIcons::eiEDITCOPY));
    QAction *magnet_info  = new QAction(sbTr("Properties of magnet"), menu);
    magnet_info->setIcon(WU->getPixmap(AppIcons::eiDOWNLOAD));
    QAction *sep1    = new QAction(menu);
    QAction *add_to_fav = new QAction(sbTr("Add to favorites"), menu);
    add_to_fav->setIcon(WU->getPixmap(AppIcons::eiBOOKMARK_ADD));
    QAction *set_rest = new QAction(sbTr("Add restriction"), rest_menu);
    QAction *rem_rest = new QAction(sbTr("Remove restriction"), rest_menu);
    open_file = new QAction(WU->getPixmap(AppIcons::eiFILETYPE_UNKNOWN), sbTr("Open file"), menu);
    open_url = new QAction(WU->getPixmap(AppIcons::eiFOLDER_BLUE), sbTr("Open directory"), menu);
    convert_epub = new QAction(WU->getPixmap(AppIcons::eiCONVERT_EPUB), sbTr("Convert to EPUB"), menu);
    delete_file = new QAction(WU->getPixmap(AppIcons::eiEDITDELETE), sbTr("Delete File"), menu);
    QAction *sep2    = new QAction(menu);
    QAction *sep3    = new QAction(menu);
    QAction *sep4    = new QAction(menu);

    actions.insert(down, Download);
    actions.insert(down_wh, DownloadWholeDir);
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
    actions.insert(convert_epub, ConvertEpub);
    actions.insert(delete_file, DeleteFile);

    magnet_menu->addActions(QList<QAction*>()
                    << magnet << magnet_web << sep3 << magnet_info);

    sep->setSeparator(true);
    sep1->setSeparator(true);
    sep2->setSeparator(true);
    sep3->setSeparator(true);
    sep4->setSeparator(true);

    menu->addActions(QList<QAction*>() << down << down_wh << sep << alter << copy_name);
    menu->addMenu(magnet_menu);
    menu->addActions(QList<QAction*>() << sep1 << add_to_fav << sep2);
    rest_menu->addActions(QList<QAction*>() << set_rest << rem_rest);
    menu->insertMenu(down_wh, down_to);
    menu->insertMenu(sep, down_wh_to);
    menu->addMenu(rest_menu);
    menu->addActions(QList<QAction*>() << open_file << open_url << convert_epub << sep4 << delete_file);
}

ShareBrowserMenu::~ShareBrowserMenu(){
    // insertMenu/addMenu transfer ownership of submenus to menu.
    delete menu;
    menu = nullptr;
    rest_menu = nullptr;
    down_to = nullptr;
    down_wh_to = nullptr;
}

ShareBrowserMenu::Action ShareBrowserMenu::exec(const dcpp::UserPtr &user, bool treePane,
                                                    bool hasDeletable, bool hasFb2){
    qDeleteAll(down_to->actions());
    qDeleteAll(down_wh_to->actions());
    down_to->clear();
    down_wh_to->clear();

    const QPixmap &dir_px = WICON(AppIcons::eiFOLDER_BLUE);
    const QString aliases = QByteArray::fromBase64(WSGET(WS_DOWNLOADTO_ALIASES).toUtf8());
    const QString paths   = QByteArray::fromBase64(WSGET(WS_DOWNLOADTO_PATHS).toUtf8());
    const QStringList a = aliases.split("\n", WULFOR_SKIP_EMPTY);
    const QStringList p = paths.split("\n", WULFOR_SKIP_EMPTY);
    const QStringList temp_pathes = DownloadToDirHistory::get();

    fillDownloadTo(down_to, temp_pathes, a, p, dir_px);
    fillDownloadTo(down_wh_to, temp_pathes, a, p, dir_px);

    const bool own = (user == ClientManager::getInstance()->getMe());
    rest_menu->setEnabled(own && treePane);
    open_file->setEnabled(own);
    open_url->setEnabled(own);
    convert_epub->setEnabled(own && hasFb2);
    delete_file->setEnabled(own && hasDeletable);

    QAction *ret = menu->exec(QCursor::pos());

    if (actions.contains(ret))
        return actions.value(ret);
    if (down_to->actions().contains(ret)) {
        target = ret->data().toString();
        return DownloadTo;
    }
    if (down_wh_to->actions().contains(ret)) {
        target = ret->data().toString();
        return DownloadWholeDirTo;
    }

    return None;
}
