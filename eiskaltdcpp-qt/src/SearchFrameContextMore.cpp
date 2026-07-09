/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SearchFrame.h"
#include "SearchFramePrivate.h"
#include "SearchModel.h"
#include "WulforUtil.h"
#include "DownloadToHistory.h"
#include "ArenaWidgetFactory.h"
#include "HubManager.h"
#include "HubFrame.h"

#include "dcpp/ClientManager.h"
#include "dcpp/SettingsManager.h"

#include <QFileDialog>
#include <QDir>
#include <QClipboard>
#include <QApplication>

#include "Magnet.h"

using namespace dcpp;

bool SearchFrame::contextMoreActions(Menu::Action act, const QModelIndexList &list)
{
    switch (act){
        case Menu::SearchTTH:
        {
            for (const auto &i : list){
                SearchItem *item = reinterpret_cast<SearchItem*>(i.internalPointer());

                if (!item->isDir){//only one file
                    SearchFrame *sf = ArenaWidgetFactory().create<SearchFrame>();

                    sf->searchAlternates(item->data(COLUMN_SF_TTH).toString());

                    break;
                }
            }

            break;
        }
        case Menu::Magnet:
        {
            QString magnets = "";
            WulforUtil *WU = WulforUtil::getInstance();

            for (const auto &i : list){
                SearchItem *item = reinterpret_cast<SearchItem*>(i.internalPointer());

                if (!item->isDir){//only files
                    const qlonglong size = item->data(COLUMN_SF_ESIZE).toLongLong();
                    const QString tth = item->data(COLUMN_SF_TTH).toString();
                    const QString name = item->data(COLUMN_SF_FILENAME).toString().trimmed();

                    QString magnet = WU->makeMagnet(name, size, tth);

                    if (!magnet.isEmpty())
                        magnets += magnet + "\n";
                }
            }

            magnets = magnets.trimmed();

            if (!magnets.isEmpty())
                qApp->clipboard()->setText(magnets, QClipboard::Clipboard);

            break;
        }
        case Menu::MagnetWeb:
        {
            QString magnets = "";
            WulforUtil *WU = WulforUtil::getInstance();

            for (const auto &i : list){
                SearchItem *item = reinterpret_cast<SearchItem*>(i.internalPointer());

                if (!item->isDir){//only files
                    const qlonglong size = item->data(COLUMN_SF_ESIZE).toLongLong();
                    const QString tth = item->data(COLUMN_SF_TTH).toString();
                    const QString name = item->data(COLUMN_SF_FILENAME).toString().trimmed();

                    QString magnet = "[magnet=\"" + WU->makeMagnet(name, size, tth) + "\"]"+name+"[/magnet]";

                    if (!magnet.isEmpty())
                        magnets += magnet + "\n";
                }
            }

            magnets = magnets.trimmed();

            if (!magnets.isEmpty())
                qApp->clipboard()->setText(magnets, QClipboard::Clipboard);

            break;
        }
        case Menu::MagnetInfo:
        {
            WulforUtil *WU = WulforUtil::getInstance();

            for (const auto &i : list){
                SearchItem *item = reinterpret_cast<SearchItem*>(i.internalPointer());

                if (!item->isDir){//only files
                    const qlonglong size = item->data(COLUMN_SF_ESIZE).toLongLong();
                    const QString &&tth = item->data(COLUMN_SF_TTH).toString();
                    const QString &&name = item->data(COLUMN_SF_FILENAME).toString().trimmed();
                    const QString &&magnet = WU->makeMagnet(name, size, tth);

                    if (!magnet.isEmpty()){
                        Magnet m(this);
                        m.setLink(magnet, Magnet::MAGNET_ACTION_SHOW_UI);
                        m.exec();
                    }
                }
            }

            break;
        }
        case Menu::Browse:
        {
            for (const auto &i : list){
                SearchItem *item = reinterpret_cast<SearchItem*>(i.internalPointer());
                VarMap params;

                if (getWholeDirParams(params, item))
                    getFileList(params, false);
            }

            break;
        }
        case Menu::MatchQueue:
        {
            for (const auto &i : list){
                SearchItem *item = reinterpret_cast<SearchItem*>(i.internalPointer());
                VarMap params;

                if (getWholeDirParams(params, item)){
                    params["FNAME"] = "";
                    getFileList(params, true);
                }
            }

            break;
        }
        default:
            return false;
    }
    return true;
}
