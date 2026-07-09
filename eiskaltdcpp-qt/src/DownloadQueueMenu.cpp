/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "DownloadQueue.h"

#include "dcpp/QueueManager.h"

#include <QAction>
#include <QCursor>
#include <QMenu>

using namespace dcpp;

DownloadQueue::Menu::Menu() : menu(new QMenu(nullptr))
{
    QMenu *menu_magnet = new QMenu(tr("Magnet"), DownloadQueue::getInstance());

    QAction *search_alt  = new QAction(tr("Search for alternates"), menu);
    QAction *copy_name   = new QAction(tr("Copy file name"), menu);
    QAction *copy_magnet = new QAction(tr("Copy magnet"), menu_magnet);
    QAction *copy_magnet_web = new QAction(tr("Copy web-magnet"), menu_magnet);
    QAction *magnet_info = new QAction(tr("Properties of magnet"), menu_magnet);
    QAction *ren_move    = new QAction(tr("Rename/Move"), menu);

    QAction *sep1 = new QAction(menu);
    sep1->setSeparator(true);

    set_prio = new QMenu(tr("Set priority"), menu);
    {
        QAction *paused = new QAction(tr("Paused"), set_prio);
        paused->setData(static_cast<int>(QueueItem::PAUSED));

        QAction *lowest = new QAction(tr("Lowest"), set_prio);
        lowest->setData(static_cast<int>(QueueItem::LOWEST));

        QAction *low    = new QAction(tr("Low"), set_prio);
        low->setData(static_cast<int>(QueueItem::LOW));

        QAction *normal = new QAction(tr("Normal"), set_prio);
        normal->setData(static_cast<int>(QueueItem::NORMAL));

        QAction *high   = new QAction(tr("High"), set_prio);
        high->setData(static_cast<int>(QueueItem::HIGH));

        QAction *highest= new QAction(tr("Highest"), set_prio);
        highest->setData(static_cast<int>(QueueItem::HIGHEST));

        set_prio->addActions(QList<QAction*>() << paused << lowest << low << normal << high << highest);
    }

    browse = new QMenu(tr("Browse files"), menu);
    send_pm = new QMenu(tr("Send private message"), menu);

    QAction *sep2 = new QAction(menu);
    sep2->setSeparator(true);

    rem_src  = new QMenu(tr("Remove source"), menu);
    rem_usr  = new QMenu(tr("Remove user"), menu);

    QAction *remove   = new QAction(tr("Remove"), menu);

    QAction *sep3 = new QAction(menu);
    sep3->setSeparator(true);

    map[search_alt] = Alternates;
    map[copy_name] = CopyFileName;
    map[copy_magnet] = Magnet;
    map[copy_magnet_web] = MagnetWeb;
    map[magnet_info] = MagnetInfo;
    map[ren_move] = RenameMove;
    map[remove] = Remove;

    menu_magnet->addActions(QList<QAction*>()
            << copy_magnet << copy_magnet_web << sep3 << magnet_info);

    menu->addAction(search_alt);
    menu->addAction(copy_name);
    menu->addMenu(menu_magnet);
    menu->addAction(ren_move);
    menu->addAction(sep1);
    menu->addMenu(set_prio);
    menu->addMenu(browse);
    menu->addMenu(send_pm);
    menu->addAction(sep2);
    menu->addMenu(rem_src);
    menu->addMenu(rem_usr);
    menu->addAction(remove);
}

DownloadQueue::Menu::~Menu(){
    delete menu;
    menu = nullptr;
}

void DownloadQueue::Menu::clearMenu(QMenu *m){
    if (!m)
        return;

    QList<QAction*> actions = m->actions();

   for (const auto &a : actions) {
       m->removeAction(a);
   }

   qDeleteAll(actions);
}

DownloadQueue::Menu::Action DownloadQueue::Menu::exec(const DownloadQueue::SourceMap &sources, const QString &target, bool multiselect){
    if (target.isEmpty() || sources.isEmpty() || !sources.contains(target))
        return None;

    arg = QVariant();

    clearMenu(browse), clearMenu(send_pm), clearMenu(rem_src), clearMenu(rem_usr);

    browse->setDisabled(multiselect);
    send_pm->setDisabled(multiselect);
    rem_src->setDisabled(multiselect);
    rem_usr->setDisabled(multiselect);

    QMap<QString, QString>  users = sources[target];

    for (const auto &key : users.keys()){
        QAction *act = new QAction(key, menu);
        act->setStatusTip(users[key]);

        browse->addAction(act);
        send_pm->addAction(act);
        rem_src->addAction(act);
        rem_usr->addAction(act);
    }

    QAction *ret = menu->exec(QCursor::pos());
    DownloadQueue::VarMap rmap;

    if (!ret)
        return None;
    else if (map.contains(ret))
        return map[ret];
    else if (set_prio->actions().contains(ret)){
        arg = ret->data();

        return SetPriority;
    }

    rmap.insert(ret->text(), ret->statusTip());
    arg = rmap;

    if (browse->actions().contains(ret))
        return Browse;
    else if (send_pm->actions().contains(ret))
        return SendPM;
    else if (rem_src->actions().contains(ret))
        return RemoveSource;
    else if (rem_usr->actions().contains(ret))
        return RemoveUser;
    else
        arg = QVariant();

    return None;
}

QVariant DownloadQueue::Menu::getArg(){
    return arg;
}
