/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "DownloadQueue.h"
#include "DownloadQueuePrivate.h"
#include "DownloadQueueModel.h"
#include "ArenaWidgetFactory.h"
#include "SearchFrame.h"
#include "WulforUtil.h"
#include "Magnet.h"

#include <QClipboard>
#include <QApplication>

using namespace dcpp;

void DownloadQueue::slotContextMenu(const QPoint &){
    QModelIndexList list = treeView_TARGET->selectionModel()->selectedRows(0);
    QList<DownloadQueueItem*> items;

    if (list.isEmpty())
        return;

    getItems(list, items);

    if (items.isEmpty())
        return;

    DownloadQueueItem *item = reinterpret_cast<DownloadQueueItem*>(items.at(0));

    QString target = item->data(COLUMN_DOWNLOADQUEUE_PATH).toString() + item->data(COLUMN_DOWNLOADQUEUE_NAME).toString();

    if (target.isEmpty())
        return;

    Q_D(DownloadQueue);

    Menu::Action act = d->menu->exec(d->sources, target, items.size() > 1);
    QVariant arg = d->menu->getArg();

    /** Now re-read selected indexes and remove broken items */
    list = treeView_TARGET->selectionModel()->selectedRows(0);

    getItems(list, items);

    if (items.isEmpty())
        return;

    switch (act){
        case Menu::Alternates:
        {
            SearchFrame *sf = ArenaWidgetFactory().create<SearchFrame>();

            for (const auto &i : items)
                sf->searchAlternates(i->data(COLUMN_DOWNLOADQUEUE_TTH).toString());

            break;
        }
        case Menu::CopyFileName:
        {
            QString names;

            for (const auto &i : items){
                const QString name = i->data(COLUMN_DOWNLOADQUEUE_NAME).toString().trimmed();

                if (!name.isEmpty())
                    names += name + "\n";
            }

            names = names.trimmed();

            if (!names.isEmpty())
                qApp->clipboard()->setText(names, QClipboard::Clipboard);

            break;
        }
        case Menu::Magnet:
        {
            QString magnet = "";

            for (const auto &i : items)
                magnet += WulforUtil::getInstance()->makeMagnet(
                        i->data(COLUMN_DOWNLOADQUEUE_NAME).toString().trimmed(),
                        i->data(COLUMN_DOWNLOADQUEUE_ESIZE).toLongLong(),
                        i->data(COLUMN_DOWNLOADQUEUE_TTH).toString()) + "\n";

            if (!magnet.isEmpty())
                qApp->clipboard()->setText(magnet, QClipboard::Clipboard);

            break;
        }
        case Menu::MagnetWeb:
        {
            QString magnet = "";

            for (const auto &i : items){
                magnet += "[magnet=\"" +
                    WulforUtil::getInstance()->makeMagnet(
                        i->data(COLUMN_DOWNLOADQUEUE_NAME).toString().trimmed(),
                        i->data(COLUMN_DOWNLOADQUEUE_ESIZE).toLongLong(),
                        i->data(COLUMN_DOWNLOADQUEUE_TTH).toString()) +
                    "\"]"+i->data(COLUMN_DOWNLOADQUEUE_NAME).toString().trimmed()+"[/magnet]\n";
            }

            if (!magnet.isEmpty())
                qApp->clipboard()->setText(magnet, QClipboard::Clipboard);

            break;
        }
        case Menu::MagnetInfo:
        {
            for (const auto &i : items){
                const QString &&magnet = WulforUtil::getInstance()->makeMagnet(
                            i->data(COLUMN_DOWNLOADQUEUE_NAME).toString().trimmed(),
                            i->data(COLUMN_DOWNLOADQUEUE_ESIZE).toLongLong(),
                            i->data(COLUMN_DOWNLOADQUEUE_TTH).toString());

                if (!magnet.isEmpty()){
                    Magnet m(this);
                    m.setLink(magnet, Magnet::MAGNET_ACTION_SHOW_UI);
                    m.exec();
                }
            }

            break;
        }
        case Menu::RenameMove:
        case Menu::SetPriority:
        case Menu::Browse:
        case Menu::SendPM:
        case Menu::RemoveSource:
        case Menu::RemoveUser:
        case Menu::Remove:
            contextMoreActions(act, items, target, arg);
            break;
        default:
            break;
    }
}
