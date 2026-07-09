/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferView.h"
#include "WulforUtil.h"

#include <QCursor>

TransferView::Menu::Menu(bool showTransferredFilesOnly, bool openEnabled, bool removeEnabled):
        menu(new QMenu(nullptr)), selectedColumn(0)
{
    WulforUtil *WU = WulforUtil::getInstance();

    QAction *browse     = new QAction(TransferView::tr("Browse files"), menu);
    browse->setIcon(WU->getPixmap(WulforUtil::eiFOLDER_BLUE));

    QAction *open_file  = new QAction(WU->getPixmap(WulforUtil::eiFOLDER_BLUE), TransferView::tr("Open file"), menu);
    QAction *open_dir   = new QAction(WU->getPixmap(WulforUtil::eiFOLDER_BLUE), TransferView::tr("Open directory"), menu);
    open_file->setEnabled(openEnabled);
    open_dir->setEnabled(openEnabled);

    QAction *search     = new QAction(TransferView::tr("Search Alternates"), menu);
    search->setIcon(WU->getPixmap(WulforUtil::eiFIND));

    QAction *match      = new QAction(TransferView::tr("Match Queue"), menu);
    match->setIcon(WU->getPixmap(WulforUtil::eiDOWN));

    QAction *send_pm    = new QAction(TransferView::tr("Send Private Message"), menu);
    send_pm->setIcon(WU->getPixmap(WulforUtil::eiMESSAGE));

    QAction *add_to_fav = new QAction(TransferView::tr("Add to favorites"), menu);
    add_to_fav->setIcon(WU->getPixmap(WulforUtil::eiBOOKMARK_ADD));

    QAction *grant      = new QAction(TransferView::tr("Grant extra slot"), menu);
    grant->setIcon(WU->getPixmap(WulforUtil::eiEDITADD));

    copy_column = new QMenu(TransferView::tr("Copy"), menu);
    copy_column->setIcon(WU->getPixmap(WulforUtil::eiEDITCOPY));

    copy_column->addAction(TransferView::tr("Users"));
    copy_column->addAction(TransferView::tr("Speed"));
    copy_column->addAction(TransferView::tr("Status"));
    copy_column->addAction(TransferView::tr("Flags"));
    copy_column->addAction(TransferView::tr("Size"));
    copy_column->addAction(TransferView::tr("Time left"));
    copy_column->addAction(TransferView::tr("Filename"));
    copy_column->addAction(TransferView::tr("Hub"));
    copy_column->addAction(TransferView::tr("IP"));
    copy_column->addAction(TransferView::tr("Encryption"));
    copy_column->addAction(TransferView::tr("Magnet"));

    QAction *sep1        = new QAction(menu);
    sep1->setSeparator(true);

    QAction *rem_queue  = new QAction(TransferView::tr("Remove Source"), menu);
    rem_queue->setIcon(WU->getPixmap(WulforUtil::eiEDITDELETE));

    QAction *remove     = new QAction(TransferView::tr("Delete File"), menu);
    remove->setIcon(WU->getPixmap(WulforUtil::eiEDITDELETE));
    remove->setEnabled(removeEnabled);

    QAction *sep3       = new QAction(menu);
    sep3->setSeparator(true);

    QAction *force    = new QAction(TransferView::tr("Force attempt"), menu);
    force->setIcon(WU->getPixmap(WulforUtil::eiCONNECT));

    QAction *close = new QAction(TransferView::tr("Close connection(s)"), menu);
    close->setIcon(WU->getPixmap(WulforUtil::eiCONNECT_NO));

    QAction *show_only_transferred_files = new QAction(TransferView::tr("Show only transferred files"), menu);
    show_only_transferred_files->setCheckable(true);
    show_only_transferred_files->setChecked(showTransferredFilesOnly);

    actions.insert(browse, Browse);
    actions.insert(open_file, OpenFile);
    actions.insert(open_dir, OpenDirectory);
    actions.insert(match, MatchQueue);
    actions.insert(send_pm, SendPM);
    actions.insert(add_to_fav, AddToFav);
    actions.insert(grant, GrantExtraSlot);
    actions.insert(rem_queue, RemoveFromQueue);
    actions.insert(remove, Remove);
    actions.insert(force, Force);
    actions.insert(close, Close);
    actions.insert(search, SearchAlternates);
    actions.insert(show_only_transferred_files, showTransferredFieldsOnly);

    menu->addActions(QList<QAction*>() << browse
                                       << open_file
                                       << open_dir
                                       << search
                                       << match
                                       << send_pm
                                       << add_to_fav
                                       << grant);
    menu->addMenu(copy_column);
    menu->addActions(QList<QAction*>() << sep1
                                       << rem_queue
                                       << remove
                                       << sep3
                                       << force
                                       << close
                                       << show_only_transferred_files
                                       );
}

TransferView::Menu::~Menu(){
    menu->deleteLater();
    copy_column->deleteLater();
}

TransferView::Menu::Action TransferView::Menu::exec(){
    QAction *ret = menu->exec(QCursor::pos());

    if (actions.contains(ret))
        return actions.value(ret);
    else if (ret){
        selectedColumn = copy_column->actions().indexOf(ret);

        return Copy;
    }

    return None;
}
