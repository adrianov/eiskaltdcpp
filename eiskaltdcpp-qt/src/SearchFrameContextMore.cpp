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
#include "SearchModel.h"
#include "search/SearchLocalPath.h"
#include "WulforUtil.h"
#include "ArenaWidgetFactory.h"

using namespace dcpp;

namespace {

SearchItem *searchItem(const QModelIndex &i)
{
    return reinterpret_cast<SearchItem*>(i.internalPointer());
}

QString searchFileMagnet(SearchItem *item)
{
    if (!item || item->isDir)
        return QString();
    return WulforUtil::getInstance()->makeMagnet(
            item->data(COLUMN_SF_FILENAME).toString().trimmed(),
            item->data(COLUMN_SF_ESIZE).toLongLong(),
            item->data(COLUMN_SF_TTH).toString());
}

void copySearchNames(const QModelIndexList &list)
{
    QString names;
    for (const auto &i : list) {
        SearchItem *item = searchItem(i);
        if (!item)
            continue;
        const QString name = (item->data(COLUMN_SF_PATH).toString()
                              + item->data(COLUMN_SF_FILENAME).toString()).trimmed();
        if (!name.isEmpty())
            names += name + QLatin1Char('\n');
    }
    WulforUtil::copyClipboard(names);
}

void copySearchMagnets(const QModelIndexList &list, bool web)
{
    QString magnets;
    for (const auto &i : list) {
        SearchItem *item = searchItem(i);
        const QString magnet = searchFileMagnet(item);
        if (magnet.isEmpty())
            continue;
        magnets += web ? WulforUtil::webMagnet(
                magnet, item->data(COLUMN_SF_FILENAME).toString().trimmed()) : magnet;
        magnets += QLatin1Char('\n');
    }
    WulforUtil::copyClipboard(magnets);
}

} // namespace

bool SearchFrame::contextLocalOpen(Menu::Action act, const QModelIndexList &list)
{
    switch (act) {
        case Menu::OpenFile:
            for (const auto &i : list)
                SearchLocalPath::openFile(searchItem(i)->localPath());
            return true;
        case Menu::OpenDirectory:
            for (const auto &i : list)
                SearchLocalPath::openDirectory(searchItem(i)->localPath());
            return true;
        case Menu::SearchTTH:
            for (const auto &i : list) {
                SearchItem *item = searchItem(i);
                if (item->isDir)
                    continue;
                ArenaWidgetFactory().create<SearchFrame>()->searchAlternates(
                        item->data(COLUMN_SF_TTH).toString());
                break;
            }
            return true;
        default:
            return false;
    }
}

bool SearchFrame::contextCopyClip(Menu::Action act, const QModelIndexList &list)
{
    switch (act) {
        case Menu::CopyFileName:
            copySearchNames(list);
            return true;
        case Menu::Magnet:
            copySearchMagnets(list, false);
            return true;
        case Menu::MagnetWeb:
            copySearchMagnets(list, true);
            return true;
        case Menu::MagnetInfo:
            for (const auto &i : list)
                WulforUtil::showMagnet(this, searchFileMagnet(searchItem(i)));
            return true;
        default:
            return false;
    }
}

bool SearchFrame::contextMoreActions(Menu::Action act, const QModelIndexList &list)
{
    if (contextLocalOpen(act, list) || contextCopyClip(act, list))
        return true;

    switch (act) {
        case Menu::Browse:
            for (const auto &i : list) {
                VarMap params;
                if (getWholeDirParams(params, searchItem(i)))
                    getFileList(params, false);
            }
            return true;
        case Menu::MatchQueue:
            for (const auto &i : list) {
                VarMap params;
                if (getWholeDirParams(params, searchItem(i))) {
                    params["FNAME"] = "";
                    getFileList(params, true);
                }
            }
            return true;
        default:
            return false;
    }
}
