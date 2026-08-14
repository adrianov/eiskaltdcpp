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

#include "DownloadQueue.h"
#include "DownloadQueuePrivate.h"
#include "DownloadQueueModel.h"
#include "ArenaWidgetFactory.h"
#include "SearchFrame.h"
#include "WulforUtil.h"

using namespace dcpp;

namespace {

QString queueMagnet(DownloadQueueItem *i)
{
    return WulforUtil::getInstance()->makeMagnet(
            i->data(COLUMN_DOWNLOADQUEUE_NAME).toString().trimmed(),
            i->data(COLUMN_DOWNLOADQUEUE_ESIZE).toLongLong(),
            i->data(COLUMN_DOWNLOADQUEUE_TTH).toString());
}

void copyQueueNames(const QList<DownloadQueueItem*> &items)
{
    QString names;
    for (const auto &i : items) {
        const QString name = (i->data(COLUMN_DOWNLOADQUEUE_PATH).toString()
                              + i->data(COLUMN_DOWNLOADQUEUE_NAME).toString()).trimmed();
        if (!name.isEmpty())
            names += name + QLatin1Char('\n');
    }
    WulforUtil::copyClipboard(names);
}

void copyQueueMagnets(const QList<DownloadQueueItem*> &items, bool web)
{
    QString magnets;
    for (const auto &i : items) {
        const QString magnet = queueMagnet(i);
        if (magnet.isEmpty())
            continue;
        magnets += web ? WulforUtil::webMagnet(
                magnet, i->data(COLUMN_DOWNLOADQUEUE_NAME).toString().trimmed()) : magnet;
        magnets += QLatin1Char('\n');
    }
    WulforUtil::copyClipboard(magnets);
}

} // namespace

bool DownloadQueue::contextCopyClip(Menu::Action act, const QList<DownloadQueueItem*> &items)
{
    switch (act) {
        case Menu::CopyFileName:
            copyQueueNames(items);
            return true;
        case Menu::Magnet:
            copyQueueMagnets(items, false);
            return true;
        case Menu::MagnetWeb:
            copyQueueMagnets(items, true);
            return true;
        case Menu::MagnetInfo:
            for (const auto &i : items)
                WulforUtil::showMagnet(this, queueMagnet(i));
            return true;
        default:
            return false;
    }
}

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

    if (act == Menu::Alternates) {
        SearchFrame *sf = ArenaWidgetFactory().create<SearchFrame>();
        for (const auto &i : items)
            sf->searchAlternates(i->data(COLUMN_DOWNLOADQUEUE_TTH).toString());
        return;
    }
    if (contextCopyClip(act, items))
        return;

    contextMoreActions(act, items, target, arg);
}
