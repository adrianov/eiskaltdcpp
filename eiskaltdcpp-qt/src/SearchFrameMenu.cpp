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

#include "SearchFrame.h"
#include "WulforUtil.h"

#include <QAction>

SearchFrame::Menu::Menu() : menu(new QMenu(nullptr))
{
    WulforUtil *WU = WulforUtil::getInstance();

    magnet_menu = new QMenu(tr("Magnet"));

    QAction *down       = new QAction(tr("Download"), nullptr);
    down->setIcon(WU->getPixmap(AppIcons::eiDOWNLOAD));

    down_to             = new QMenu(tr("Download to..."));
    down_to->setIcon(WU->getPixmap(AppIcons::eiDOWNLOAD_AS));

    QAction *down_wh    = new QAction(tr("Download Whole Directory"), nullptr);
    down_wh->setIcon(WU->getPixmap(AppIcons::eiDOWNLOAD));

    down_wh_to          = new QMenu(tr("Download Whole Directory to..."));
    down_wh_to->setIcon(WU->getPixmap(AppIcons::eiDOWNLOAD_AS));

    QAction *open_file  = new QAction(tr("Open file"), nullptr);
    open_file->setIcon(WU->getPixmap(AppIcons::eiFOLDER_BLUE));

    QAction *open_dir   = new QAction(tr("Open directory"), nullptr);
    open_dir->setIcon(WU->getPixmap(AppIcons::eiFOLDER_BLUE));

    QAction *sep        = new QAction(menu);
    sep->setSeparator(true);

    QAction *find_tth   = new QAction(tr("Search TTH"), nullptr);
    find_tth->setIcon(WU->getPixmap(AppIcons::eiFILEFIND));

    QAction *copy_name  = new QAction(tr("Copy file name"), nullptr);
    copy_name->setIcon(WU->getPixmap(AppIcons::eiEDITCOPY));

    QAction *magnet     = new QAction(tr("Copy magnet"), nullptr);
    magnet->setIcon(WU->getPixmap(AppIcons::eiEDITCOPY));

    QAction *magnet_web     = new QAction(tr("Copy web-magnet"), nullptr);
    magnet_web->setIcon(WU->getPixmap(AppIcons::eiEDITCOPY));

    QAction *magnet_info    = new QAction(tr("Properties of magnet"), nullptr);
    magnet_info->setIcon(WU->getPixmap(AppIcons::eiDOWNLOAD));

    QAction *browse     = new QAction(tr("Browse files"), nullptr);
    browse->setIcon(WU->getPixmap(AppIcons::eiFOLDER_BLUE));

    QAction *match      = new QAction(tr("Match Queue"), nullptr);
    match->setIcon(WU->getPixmap(AppIcons::eiDOWN));

    QAction *send_pm    = new QAction(tr("Send Private Message"), nullptr);
    send_pm->setIcon(WU->getPixmap(AppIcons::eiMESSAGE));

    QAction *add_to_fav = new QAction(tr("Add to favorites"), nullptr);
    add_to_fav->setIcon(WU->getPixmap(AppIcons::eiBOOKMARK_ADD));

    QAction *grant      = new QAction(tr("Grant extra slot"), nullptr);
    grant->setIcon(WU->getPixmap(AppIcons::eiEDITADD));

    QAction *sep1       = new QAction(menu);
    sep1->setSeparator(true);

    QAction *sep2       = new QAction(menu);
    sep2->setSeparator(true);

    QAction *sep3       = new QAction(menu);
    sep3->setSeparator(true);

    QAction *rem_queue  = new QAction(tr("Remove from Queue"), nullptr);
    rem_queue->setIcon(WU->getPixmap(AppIcons::eiEDITDELETE));

    QAction *rem        = new QAction(tr("Remove"), nullptr);
    rem->setIcon(WU->getPixmap(AppIcons::eiEDITDELETE));

    black_list_menu     = new QMenu(tr("Blacklist..."));
    black_list_menu->setIcon(WU->getPixmap(AppIcons::eiFILTER));

    QAction *blacklist = new QAction(tr("Blacklist"), nullptr);
    blacklist->setIcon(WU->getPixmap(AppIcons::eiFILTER));

    QAction *add_to_blacklist = new QAction(tr("Add to Blacklist"), nullptr);
    add_to_blacklist->setIcon(WU->getPixmap(AppIcons::eiEDITADD));

    black_list_menu->addActions(QList<QAction*>() << add_to_blacklist << blacklist);
    magnet_menu->addActions(QList<QAction*>() << magnet << magnet_web << sep3 << magnet_info);

    actions.insert(down, Download);
    actions.insert(down_wh, DownloadWholeDir);
    actions.insert(open_file, OpenFile);
    actions.insert(open_dir, OpenDirectory);
    actions.insert(find_tth, SearchTTH);
    actions.insert(copy_name, CopyFileName);
    actions.insert(magnet, Magnet);
    actions.insert(magnet_web, MagnetWeb);
    actions.insert(magnet_info, MagnetInfo);
    actions.insert(browse, Browse);
    actions.insert(match, MatchQueue);
    actions.insert(send_pm, SendPM);
    actions.insert(add_to_fav, AddToFav);
    actions.insert(grant, GrantExtraSlot);
    actions.insert(rem_queue, RemoveFromQueue);
    actions.insert(rem, Remove);
    actions.insert(blacklist, Blacklist);
    actions.insert(add_to_blacklist, AddToBlacklist);

    action_list << down << down_wh << open_file << open_dir << sep << find_tth << copy_name << browse << match
                << send_pm << add_to_fav << grant << sep1 << rem_queue << rem << sep2;
}

SearchFrame::Menu::~Menu(){
    qDeleteAll(action_list);
    action_list.clear();

    magnet_menu->deleteLater();
    menu->deleteLater();
    down_to->deleteLater();
    down_wh_to->deleteLater();
    black_list_menu->deleteLater();
}
