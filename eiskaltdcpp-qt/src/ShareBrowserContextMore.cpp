/***************************************************************************
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
#include "Magnet.h"
#include "ArenaWidgetFactory.h"


#include <QClipboard>
#include <QDir>
#include <QApplication>

using namespace dcpp;

void ShareBrowser::contextMoreActions(Menu::Action act, const QModelIndexList &list)
{
    switch (act){
        case Menu::Alternates:
        {
            for (const auto &index : list){
                FileBrowserItem *item = reinterpret_cast<FileBrowserItem*>(index.internalPointer());

                if (item->file){//search alternates only for files
                    QString tth = item->data(COLUMN_FILEBROWSER_TTH).toString();
                    SearchFrame *sf = ArenaWidgetFactory().create<SearchFrame>();

                    sf->searchAlternates(tth);

                    break;//just one file
                }
            }

            break;
        }
        case Menu::CopyFileName:
        {
            QString names;

            for (const auto &index : list){
                FileBrowserItem *item = reinterpret_cast<FileBrowserItem*>(index.internalPointer());
                const QString name = item->data(COLUMN_FILEBROWSER_NAME).toString().trimmed();

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
            QString magnets = "";
            QString path, tth, magnet;
            qlonglong size;

            for (const auto &index : list){
                FileBrowserItem *item = reinterpret_cast<FileBrowserItem*>(index.internalPointer());

                path = item->data(COLUMN_FILEBROWSER_NAME).toString().trimmed();
                tth  = item->data(COLUMN_FILEBROWSER_TTH).toString();
                size = item->data(COLUMN_FILEBROWSER_ESIZE).toLongLong();

                magnet = WulforUtil::getInstance()->makeMagnet(path, size, tth);

                if (!magnet.isEmpty())
                    magnets += (magnet + "\n");
            }

            magnets = magnets.trimmed();

            qApp->clipboard()->setText(magnets, QClipboard::Clipboard);

            break;
        }
        case Menu::MagnetWeb:
        {
            QString magnets = "";
            QString path, tth, magnet;
            qlonglong size;

            for (const auto &index : list){
                FileBrowserItem *item = reinterpret_cast<FileBrowserItem*>(index.internalPointer());

                path = item->data(COLUMN_FILEBROWSER_NAME).toString().trimmed();
                tth  = item->data(COLUMN_FILEBROWSER_TTH).toString();
                size = item->data(COLUMN_FILEBROWSER_ESIZE).toLongLong();

                magnet = "[magnet=\"" + WulforUtil::getInstance()->makeMagnet(path, size, tth) + "\"]"+path+"[/magnet]";

                if (!magnet.isEmpty())
                    magnets += (magnet + "\n");
            }

            magnets = magnets.trimmed();

            qApp->clipboard()->setText(magnets, QClipboard::Clipboard);

            break;
        }
        case Menu::MagnetInfo:
        {
            QString path, tth, magnet;
            qlonglong size;

            for (const auto &index : list){
                FileBrowserItem *item = reinterpret_cast<FileBrowserItem*>(index.internalPointer());

                path = item->data(COLUMN_FILEBROWSER_NAME).toString().trimmed();
                tth  = item->data(COLUMN_FILEBROWSER_TTH).toString();
                size = item->data(COLUMN_FILEBROWSER_ESIZE).toLongLong();

                magnet = WulforUtil::getInstance()->makeMagnet(path, size, tth);

                if (!magnet.isEmpty()){
                    Magnet m(this);
                    m.setLink(magnet, Magnet::MAGNET_ACTION_SHOW_UI);
                    m.exec();
                }
            }

            break;
        }
        case Menu::AddToFav:
        case Menu::AddRestrinction:
        case Menu::RemoveRestriction:
        case Menu::OpenUrl:
            contextUserActions(act, list);
            break;
        default: break;
    }
}
