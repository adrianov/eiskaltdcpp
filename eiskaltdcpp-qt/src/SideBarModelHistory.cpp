/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "SideBar.h"
#include "WulforSettings.h"
#include "MainWindow.h"

#define RETRANSLATE_ROOT_EL(text, map, type_el) \
    do { \
        if ((map).contains(ArenaWidget::type_el) && (map)[ArenaWidget::type_el]) \
            (map)[ArenaWidget::type_el]->title = text; \
    } while (0)

void SideBarModel::historyPop(){
    if (historyStack.empty())
        return;

    historyStack.pop();//remove last widget

    if (historyStack.isEmpty())
        return;

    ArenaWidget *awgt = historyStack.pop();

    emit mapWidget(awgt);
}

void SideBarModel::historyPush(ArenaWidget *awgt){
    historyPurge(awgt);

    historyStack.push(awgt);
}

bool SideBarModel::historyAtTop(ArenaWidget *awgt){
    return (!historyStack.isEmpty() && historyStack.top() == awgt);
}

void SideBarModel::historyPurge(ArenaWidget *awgt){
    if (historyStack.indexOf(awgt) >= 0)
        historyStack.remove(historyStack.indexOf(awgt));
}

void SideBarModel::slotIndexClicked(const QModelIndex &i){
    if (!(i.isValid() && i.internalPointer()))
        return;

    SideBarItem *item = reinterpret_cast<SideBarItem*>(i.internalPointer());
    ArenaWidget *awgt = item->getWidget();

    if (items.contains(awgt))
        emit mapWidget(awgt);
    else {
        ArenaWidget::Role role = roots.key(item);
        ArenaWidget *awgt = MainWindow::getInstance()->widgetForRole(role);

        emit mapWidget(awgt);
    }
}

void SideBarModel::slotSettingsChanged(const QString &key, const QString &value){
    Q_UNUSED(value)
    if (key == WS_TRANSLATION_FILE){
        RETRANSLATE_ROOT_EL(tr("Hubs"),             roots,  Hub);
        RETRANSLATE_ROOT_EL(tr("Private Messages"), roots,  PrivateMessage);
        RETRANSLATE_ROOT_EL(tr("Search"),           roots,  Search);
        RETRANSLATE_ROOT_EL(tr("Share Browsers"),   roots,  ShareBrowser);
        RETRANSLATE_ROOT_EL(tr("ADLSearch"),        roots,  ADLS);
        RETRANSLATE_ROOT_EL(tr("Download Queue"),   roots,  Downloads);
        RETRANSLATE_ROOT_EL(tr("Finished Uploads"), roots,  FinishedUploads);
        RETRANSLATE_ROOT_EL(tr("Finished Downloads"),roots, FinishedDownloads);
        RETRANSLATE_ROOT_EL(tr("Favorite Hubs"),    roots,  FavoriteHubs);
        RETRANSLATE_ROOT_EL(tr("Favorite Users"),   roots,  FavoriteUsers);
        RETRANSLATE_ROOT_EL(tr("Public Hubs"),      roots,  PublicHubs);
        RETRANSLATE_ROOT_EL(tr("Secretary"),        roots,  Secretary);
        RETRANSLATE_ROOT_EL(tr("Search Spy"),       roots,  SearchSpy);
        RETRANSLATE_ROOT_EL(tr("Other Widgets"),    roots,  CustomWidget);
        RETRANSLATE_ROOT_EL(tr("Queued Users"),     roots,  QueuedUsers);
        RETRANSLATE_ROOT_EL(tr("Debug Console"),    roots,  CmdDebug);
    }
}

