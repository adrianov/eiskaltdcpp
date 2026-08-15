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

#include "ShareBrowser.h"
#include "WulforUtil.h"
#include "FileBrowserModel.h"
#include "SearchFrame.h"
#include "ArenaWidgetFactory.h"

using namespace dcpp;

namespace {

FileBrowserItem *browserItem(const QModelIndex &index)
{
    return reinterpret_cast<FileBrowserItem*>(index.internalPointer());
}

QString listingCopyName(DirectoryListing &listing, FileBrowserItem *item)
{
    if (!item)
        return QString();
    if (item->file)
        return _q(listing.getPath(item->file) + item->file->getName());
    if (item->dir) {
        QString name = _q(listing.getPath(item->dir));
        if (name.endsWith(QLatin1Char('\\')))
            name.chop(1);
        if (!name.isEmpty())
            return name;
    }
    return item->data(COLUMN_FILEBROWSER_NAME).toString().trimmed();
}

QString shareFileMagnet(FileBrowserItem *item)
{
    if (!item)
        return QString();
    return WulforUtil::getInstance()->makeMagnet(
            item->data(COLUMN_FILEBROWSER_NAME).toString().trimmed(),
            item->data(COLUMN_FILEBROWSER_ESIZE).toLongLong(),
            item->data(COLUMN_FILEBROWSER_TTH).toString());
}

void copyShareNames(DirectoryListing &listing, const QModelIndexList &list)
{
    QString names;
    for (const auto &index : list) {
        const QString name = listingCopyName(listing, browserItem(index));
        if (!name.isEmpty())
            names += name + QLatin1Char('\n');
    }
    WulforUtil::copyClipboard(names);
}

void copyShareMagnets(const QModelIndexList &list, bool web)
{
    QString magnets;
    for (const auto &index : list) {
        FileBrowserItem *item = browserItem(index);
        const QString magnet = shareFileMagnet(item);
        if (magnet.isEmpty())
            continue;
        magnets += web ? WulforUtil::webMagnet(
                magnet, item->data(COLUMN_FILEBROWSER_NAME).toString().trimmed()) : magnet;
        magnets += QLatin1Char('\n');
    }
    WulforUtil::copyClipboard(magnets);
}

} // namespace

bool ShareBrowser::contextCopyClip(ShareBrowserMenu::Action act, const QModelIndexList &list)
{
    switch (act) {
        case ShareBrowserMenu::CopyFileName:
            copyShareNames(listing, list);
            return true;
        case ShareBrowserMenu::Magnet:
            copyShareMagnets(list, false);
            return true;
        case ShareBrowserMenu::MagnetWeb:
            copyShareMagnets(list, true);
            return true;
        case ShareBrowserMenu::MagnetInfo:
            for (const auto &index : list)
                WulforUtil::showMagnet(this, shareFileMagnet(browserItem(index)));
            return true;
        default:
            return false;
    }
}

void ShareBrowser::contextMoreActions(ShareBrowserMenu::Action act, const QModelIndexList &list)
{
    if (contextCopyClip(act, list))
        return;

    switch (act) {
        case ShareBrowserMenu::Alternates:
            for (const auto &index : list) {
                FileBrowserItem *item = browserItem(index);
                if (!item || !item->file)
                    continue;
                ArenaWidgetFactory().create<SearchFrame>()->searchAlternates(
                        item->data(COLUMN_FILEBROWSER_TTH).toString());
                break;
            }
            break;
        default:
            contextUserActions(act, list);
            break;
    }
}
