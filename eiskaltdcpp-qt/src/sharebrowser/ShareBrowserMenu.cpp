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
#include "downloadto/DownloadToMenu.h"
#include "fb2epub/Fb2EpubExport.h"
#include "WulforUtil.h"

#include "dcpp/ClientManager.h"

#include <QCoreApplication>
#include <QCursor>

using namespace dcpp;

namespace {

QString sbTr(const char *s)
{
    return QCoreApplication::translate("ShareBrowser", s);
}

QString wholeDirTr(const char *s)
{
    return QCoreApplication::translate("SearchFrame", s);
}

} // namespace

ShareBrowserMenu::ShareBrowserMenu() : menu(new QMenu(nullptr))
{
    WulforUtil *WU = WulforUtil::getInstance();

    rest_menu = new QMenu(sbTr("Restrictions"));
    rest_menu->setIcon(WU->getPixmap(AppIcons::eiFILTER));
    QMenu *magnet_menu = new QMenu(sbTr("Magnet"), menu);
    magnet_menu->setIcon(WU->getPixmap(AppIcons::eiMAGNET));

    QAction *down = new QAction(sbTr("Download"), menu);
    down->setIcon(WU->getPixmap(AppIcons::eiDOWNLOAD));
    down_to = new DownloadToMenu(sbTr("Download to..."), sbTr("Browse"));
    QAction *down_wh = new QAction(wholeDirTr("Download Whole Directory"), menu);
    down_wh->setIcon(WU->getPixmap(AppIcons::eiDOWNLOAD));
    down_wh_to = new DownloadToMenu(wholeDirTr("Download Whole Directory to..."),
                                    sbTr("Browse"));
    QAction *sep = new QAction(menu);
    QAction *alter = new QAction(sbTr("Search for alternates"), menu);
    alter->setIcon(WU->getPixmap(AppIcons::eiFILEFIND));
    QAction *copy_name = new QAction(sbTr("Copy file name"), menu);
    copy_name->setIcon(WU->getPixmap(AppIcons::eiEDITCOPY));
    QAction *magnet = new QAction(sbTr("Copy magnet"), menu);
    magnet->setIcon(WU->getPixmap(AppIcons::eiEDITCOPY));
    QAction *magnet_web = new QAction(sbTr("Copy web-magnet"), menu);
    magnet_web->setIcon(WU->getPixmap(AppIcons::eiEDITCOPY));
    QAction *magnet_info = new QAction(sbTr("Properties of magnet"), menu);
    magnet_info->setIcon(WU->getPixmap(AppIcons::eiDOWNLOAD));
    QAction *sep1 = new QAction(menu);
    QAction *add_to_fav = new QAction(sbTr("Add to favorites"), menu);
    add_to_fav->setIcon(WU->getPixmap(AppIcons::eiBOOKMARK_ADD));
    QAction *set_rest = new QAction(sbTr("Add restriction"), rest_menu);
    set_rest->setIcon(WU->getPixmap(AppIcons::eiEDITADD));
    QAction *rem_rest = new QAction(sbTr("Remove restriction"), rest_menu);
    rem_rest->setIcon(WU->getPixmap(AppIcons::eiEDITDELETE));
    open_file = new QAction(WU->getPixmap(AppIcons::eiFILETYPE_UNKNOWN), sbTr("Open file"), menu);
    open_url = new QAction(WU->getPixmap(AppIcons::eiFOLDER_BLUE), sbTr("Open directory"), menu);
    convert_epub = new QAction(WU->getPixmap(AppIcons::eiCONVERT_EPUB), sbTr("Convert to EPUB"), menu);
    rename_folder = new QAction(WU->getPixmap(AppIcons::eiEDIT), sbTr("Rename Folder"), menu);
    delete_file = new QAction(WU->getPixmap(AppIcons::eiEDITDELETE), sbTr("Delete File"), menu);
    delete_other_copies = new QAction(WU->getPixmap(AppIcons::eiEDITDELETE),
                                      sbTr("Delete Other Copy"), menu);
    delete_other_copies->setVisible(false);
    delete_whole_dir = new QAction(WU->getPixmap(AppIcons::eiEDITDELETE),
                                   sbTr("Delete Whole Directory"), menu);
    QAction *sep2 = new QAction(menu);
    QAction *sep3 = new QAction(menu);
    QAction *sep4 = new QAction(menu);

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
    actions.insert(rename_folder, RenameFolder);
    actions.insert(delete_file, DeleteFile);
    actions.insert(delete_other_copies, DeleteOtherCopies);
    actions.insert(delete_whole_dir, DeleteWholeDir);

    magnet_menu->addActions(QList<QAction*>() << magnet << magnet_web << sep3 << magnet_info);

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
    menu->addActions(QList<QAction*>() << open_file << open_url << convert_epub << sep4
                     << rename_folder << delete_file << delete_other_copies << delete_whole_dir);
}

ShareBrowserMenu::~ShareBrowserMenu(){
    // insertMenu/addMenu transfer ownership of submenus to menu.
    delete menu;
    menu = nullptr;
    rest_menu = nullptr;
    down_to = nullptr;
    down_wh_to = nullptr;
}

ShareBrowserMenu::Action ShareBrowserMenu::exec(const dcpp::UserPtr &user, const Flags &flags){
    down_to->refill();
    down_wh_to->refill();

    const bool own = (user == ClientManager::getInstance()->getMe());
    const bool canConvert = own && flags.fb2;
    rest_menu->setEnabled(own && flags.treePane);
    open_file->setEnabled(own);
    open_url->setEnabled(own);
    convert_epub->setEnabled(canConvert);
    convert_epub->setVisible(canConvert);
    rename_folder->setEnabled(own && flags.renameFolder);
    rename_folder->setVisible(own && flags.renameFolder);
    delete_file->setEnabled(own && flags.deletable);
    const bool canDeleteOthers = own && flags.otherCopies > 0;
    delete_other_copies->setVisible(canDeleteOthers);
    delete_other_copies->setEnabled(canDeleteOthers);
    if (canDeleteOthers) {
        if (flags.otherCopies == 1)
            delete_other_copies->setText(sbTr("Delete Other Copy"));
        else
            delete_other_copies->setText(sbTr("Delete Other Copies (%1)").arg(flags.otherCopies));
    }
    delete_whole_dir->setEnabled(own && flags.deleteWholeDir);

    if (canConvert && Fb2EpubExport::convertIsDefaultOpen())
        menu->setDefaultAction(convert_epub);
    else if (own)
        menu->setDefaultAction(open_file);
    else
        menu->setDefaultAction(nullptr);

    QAction *ret = menu->exec(QCursor::pos());

    if (actions.contains(ret))
        return actions.value(ret);
    if (down_to->takeTarget(ret, target))
        return DownloadTo;
    if (down_wh_to->takeTarget(ret, target))
        return DownloadWholeDirTo;

    return None;
}
