/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "SideBar.h"
#include "WulforUtil.h"
#include "WulforSettings.h"

#define CREATE_ROOT_EL(a, b, c, d, e) \
    do { \
    SideBarItem *root = new SideBarItem(nullptr, (a)); \
    root->pixmap = WU->getPixmap(WulforUtil::b); \
    root->title  = (c); \
    (d).insert(ArenaWidget::e, root); \
    (a)->appendChild(root); \
    } while (0)

SideBarItem::SideBarItem(ArenaWidget *wgt, SideBarItem *parent)
    : parentItem(parent)
    , awgt(wgt)
{
}

SideBarItem::~SideBarItem()
{
    qDeleteAll(childItems);
    childItems.clear();
}

SideBarModel::SideBarModel(QObject *parent) :
    QAbstractItemModel(parent)
{
    rootItem = new SideBarItem(nullptr, nullptr);

    WulforUtil *WU = WulforUtil::getInstance();

    CREATE_ROOT_EL(rootItem, eiSERVER,      tr("Hubs"),             roots,  Hub);
    CREATE_ROOT_EL(rootItem, eiUSERS,       tr("Private Messages"), roots,  PrivateMessage);
    CREATE_ROOT_EL(rootItem, eiFILEFIND,    tr("Search"),           roots,  Search);
    CREATE_ROOT_EL(rootItem, eiOWN_FILELIST,tr("Share Browsers"),   roots,  ShareBrowser);
    CREATE_ROOT_EL(rootItem, eiADLS,        tr("ADLSearch"),        roots,  ADLS);
    CREATE_ROOT_EL(rootItem, eiDOWNLOAD,    tr("Download Queue"),   roots,  Downloads);
    CREATE_ROOT_EL(rootItem, eiUSERS,       tr("Queued Users"),     roots,  QueuedUsers);
    CREATE_ROOT_EL(rootItem, eiUPLIST,      tr("Finished Uploads"), roots,  FinishedUploads);
    CREATE_ROOT_EL(rootItem, eiDOWNLIST,    tr("Finished Downloads"),roots, FinishedDownloads);
    CREATE_ROOT_EL(rootItem, eiFAVSERVER,   tr("Favorite Hubs"),    roots,  FavoriteHubs);
    CREATE_ROOT_EL(rootItem, eiFAVUSERS,    tr("Favorite Users"),   roots,  FavoriteUsers);
    CREATE_ROOT_EL(rootItem, eiSERVER,      tr("Public Hubs"),      roots,  PublicHubs);
    CREATE_ROOT_EL(rootItem, eiMAGNET,      tr("Secretary"),        roots,  Secretary);
    CREATE_ROOT_EL(rootItem, eiSPY,         tr("Search Spy"),       roots,  SearchSpy);
    CREATE_ROOT_EL(rootItem, eiCONSOLE,     tr("Debug Console"),    roots,  CmdDebug);
    //CREATE_ROOT_EL(rootItem, eiSERVER,    tr("Hub Manager"),      roots,  HubManager);
    CREATE_ROOT_EL(rootItem, eiGUI,         tr("Other Widgets"),    roots,  CustomWidget);

    connect(WulforSettings::getInstance(), SIGNAL(strValueChanged(QString,QString)),
            this, SLOT(slotSettingsChanged(QString,QString)));
}

SideBarModel::~SideBarModel()
{
    if (rootItem)
        delete rootItem;
}

int SideBarModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<SideBarItem*>(parent.internalPointer())->columnCount();
    else
        return rootItem->columnCount();
}
