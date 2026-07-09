/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TabFrame.h"
#include "FlowLayout.h"
#include "TabButton.h"
#include "ArenaWidgetManager.h"
#include "DebugHelper.h"
#include "WulforUtil.h"

#if QT_VERSION >= 0x050000
#include <QtWidgets>
#else
#include <QtGui>
#endif

void TabFrame::historyPush(ArenaWidget *awgt){
    DEBUG_BLOCK
    
    historyPurge(awgt);

    history.push_back(awgt);
}

void TabFrame::historyPurge(ArenaWidget *awgt){
    DEBUG_BLOCK
    
    if (history.contains(awgt))
        history.removeAt(history.indexOf(awgt));
}

void TabFrame::historyPop(){
    DEBUG_BLOCK
    
    if (history.isEmpty() && fr_layout->count() > 0){
        QLayoutItem *item = fr_layout->itemAt(0);

        if (!item)
            return;

        TabButton *btn = qobject_cast<TabButton*>(item->widget());

        if (btn)
            ArenaWidgetManager::getInstance()->activate(tbtn_map[btn]);

        return;
    }
    else if (history.isEmpty()){
        ArenaWidgetManager::getInstance()->activate(nullptr);
        
        return;
    }

    ArenaWidget *awgt = history.takeLast();

    ArenaWidgetManager::getInstance()->activate(awgt);
}

void TabFrame::buttonClicked(){
    DEBUG_BLOCK
    
    TabButton *btn = qobject_cast<TabButton*>(sender());

    if (!(btn && tbtn_map.contains(btn)))
        return;

    btn->setFocus();

    ArenaWidgetManager::getInstance()->activate(tbtn_map[btn]);
}

void TabFrame::closeRequsted() {
    DEBUG_BLOCK
    
    TabButton *btn = qobject_cast<TabButton*>(sender());

    if (!(btn && tbtn_map.contains(btn)))
        return;

    ArenaWidget *awgt = const_cast<ArenaWidget*>(tbtn_map[btn]);
    ArenaWidgetManager::getInstance()->rem(awgt);
}

