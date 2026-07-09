/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ToolBar.h"
#include "WulforUtil.h"
#include "MainWindow.h"
#include "PMWindow.h"
#include "WulforSettings.h"
#include "ArenaWidgetManager.h"

#include <QMenu>
#include <typeinfo>

void ToolBar::insertWidget(ArenaWidget *awgt){
    if (!(awgt && awgt->getWidget()) || (awgt->state() & ArenaWidget::Hidden) || map.contains(awgt))
        return;

    int index = tabbar->addTab(awgt->getPixmap(), awgt->getArenaShortTitle().left(32));

    if (awgt->toolButton())
        awgt->toolButton()->setChecked(true);

    if (index >= 0){
        map.insert(awgt, index);

        if (tabbar->isHidden())
            tabbar->show();

        if (!(typeid(*awgt) == typeid(PMWindow) && WBGET(WB_CHAT_KEEPFOCUS)))
            tabbar->setCurrentIndex(index);
    }
}

void ToolBar::removeWidget(ArenaWidget *awgt){
    if (!awgt || !awgt->getWidget() || !map.contains(awgt))
        return;

    int index = map.value(awgt);

    if (index >= 0){
        map.erase(map.find(awgt));

        rebuildIndexes(index);

        if (map.isEmpty())
            tabbar->hide();

        tabbar->removeTab(index);

        if (awgt->toolButton())
            awgt->toolButton()->setChecked(false);
    }
}

void ToolBar::updated ( ArenaWidget *awgt ) {
    if (!awgt)
        return;
    
    if ( awgt->state() & ArenaWidget::Hidden ) {
        removeWidget ( awgt );
    } else if ( !map.contains(awgt)) {
        insertWidget ( awgt );
    }
}

void ToolBar::slotIndexChanged(int index){
    if (index < 0)
        return;

    ArenaWidget *awgt = findWidgetForIndex(index);

    if (!awgt || !awgt->getWidget())
        return;

    ArenaWidgetManager::getInstance()->activate(awgt);
}

void ToolBar::toggled ( ArenaWidget *awgt) {
    if (!awgt)
        return;
        
    if (!(awgt->state() & ArenaWidget::Singleton))
        return;
    
    if (awgt->state() & ArenaWidget::Hidden)
        ArenaWidgetManager::getInstance()->activate(awgt);
    else
        ArenaWidgetManager::getInstance()->rem(awgt);
}

void ToolBar::slotTabMoved(int from, int to){
    ArenaWidget *from_wgt = nullptr;
    ArenaWidget *to_wgt   = nullptr;

    for (auto it = map.begin(); it != map.end(); ++it){
        if (it.value() == from){
            from_wgt = it.key();
        }
        else if (it.value() == to)
            to_wgt = it.key();

        if (to_wgt && from_wgt){
            map[to_wgt] = from;
            map[from_wgt] = to;

            slotIndexChanged(tabbar->currentIndex());

            return;
        }
    }
}

void ToolBar::slotClose(int index){
    if (index < 0)
        return;

    ArenaWidget *awgt = findWidgetForIndex(index);

    if (!awgt || !awgt->getWidget())
        return;

    ArenaWidgetManager::getInstance()->rem(awgt);
}

void ToolBar::slotContextMenu(const QPoint &p){
    int tab = tabbar->tabAt(p);
    ArenaWidget *awgt = findWidgetForIndex(tab);

    if (!awgt){
        QMenu *m = new QMenu(this);
        QAction *act = new QAction(tr("Show close buttons"), m);

        act->setCheckable(true);
        act->setChecked(WBGET(WB_APP_TBAR_SHOW_CL_BTNS));

        m->addAction(act);

        if (m->exec(QCursor::pos())){
            WBSET(WB_APP_TBAR_SHOW_CL_BTNS, act->isChecked());
            tabbar->setTabsClosable(act->isChecked());
        }

        m->deleteLater();

        return;
    }

    QMenu *m = awgt->getMenu();

    if (m)
        m->exec(QCursor::pos());
}

void ToolBar::slotShorcuts(){
    QShortcut *sh = qobject_cast<QShortcut*>(sender());

    if (!sh)
        return;

    int index = shortcuts.indexOf(sh);

    if (index >= 0 && tabbar->count() >= (index + 1))
        tabbar->setCurrentIndex(index);
}

ArenaWidget *ToolBar::findWidgetForIndex(const int index){
    if (index < 0)
        return nullptr;

    for (const auto &k : map.keys()) {
        if (map[k] == index)
            return const_cast<ArenaWidget*>(k);
    }

    return nullptr;
}

