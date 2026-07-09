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

void TabFrame::nextTab(){
    DEBUG_BLOCK
    
    TabButton *next = nullptr;

    for (int i = 0; i < fr_layout->count(); i++){
        TabButton *t = qobject_cast<TabButton*>(fr_layout->itemAt(i)->widget());

        if (t && t->isChecked()){
            if (i == (fr_layout->count()-1)){
                next = qobject_cast<TabButton*>(fr_layout->itemAt(0)->widget());
                break;
            }

            next = qobject_cast<TabButton*>(fr_layout->itemAt(i+1)->widget());
            break;
        }
    }

    if (!next)
        return;

    ArenaWidgetManager::getInstance()->activate(tbtn_map[next]);
}

void TabFrame::prevTab(){
    DEBUG_BLOCK
    
    TabButton *next = nullptr;

    for (int i = 0; i < fr_layout->count(); i++){
        TabButton *t = qobject_cast<TabButton*>(fr_layout->itemAt(i)->widget());

        if (t && t->isChecked()){
            if (!i){
                next = qobject_cast<TabButton*>(fr_layout->itemAt(fr_layout->count()-1)->widget());
                break;
            }

            next = qobject_cast<TabButton*>(fr_layout->itemAt(i-1)->widget());
            break;
        }
    }

    if (!next)
        return;

   ArenaWidgetManager::getInstance()->activate(tbtn_map[next]);
}

void TabFrame::slotShorcuts(){
    DEBUG_BLOCK
    
    QShortcut *sh = qobject_cast<QShortcut*>(sender());

    if (!sh)
        return;

    int index = shortcuts.indexOf(sh);

    if (index >= 0 && fr_layout->count() >= (index + 1)){
        TabButton *next = qobject_cast<TabButton*>(fr_layout->itemAt(index)->widget());

        if (!next)
            return;

        ArenaWidgetManager::getInstance()->activate(tbtn_map[next]);
    }
}

void TabFrame::slotContextMenu() {
    DEBUG_BLOCK
    
    TabButton *btn = qobject_cast<TabButton*>(sender());

    if (!(btn && tbtn_map.contains(btn)))
        return;

    ArenaWidget *awgt = const_cast<ArenaWidget*>(tbtn_map[btn]);

    if (awgt) {
        QMenu *widget_menu = awgt->getMenu();
        if (widget_menu) {
            widget_menu->exec(btn->mapToGlobal(btn->rect().bottomLeft()));
        } else {
            widget_menu = new QMenu(this);
            widget_menu->addAction(WulforUtil::getInstance()->getPixmap(WulforUtil::eiEDITDELETE), tr("Close"));

            if (widget_menu->exec(QCursor::pos()))
                ArenaWidgetManager::getInstance()->rem(awgt);

            delete widget_menu;
        }
    }
}

void TabFrame::slotDropped(TabButton *dropped){
    DEBUG_BLOCK
    
    TabButton *on = qobject_cast<TabButton*>(sender());

    if (!(on && dropped && on != dropped))
        return;

    fr_layout->place(on, dropped);
}

void TabFrame::moveLeft(){
    DEBUG_BLOCK
    
    for (int i = 0; i < fr_layout->count(); i++){
        QLayoutItem *item = const_cast<QLayoutItem*>(fr_layout->itemAt(i));
        TabButton *t = qobject_cast<TabButton*>(item->widget());

        if (t && t->isChecked()){
            fr_layout->moveLeft(item);

            break;
        }
    }
}

void TabFrame::moveRight(){
    DEBUG_BLOCK
    
    for (int i = 0; i < fr_layout->count(); i++){
        QLayoutItem *item = const_cast<QLayoutItem*>(fr_layout->itemAt(i));
        TabButton *t = qobject_cast<TabButton*>(item->widget());

        if (t && t->isChecked()){
            fr_layout->moveRight(item);

            break;
        }
    }
}

void TabFrame::toggled ( ArenaWidget* awgt ) {
    DEBUG_BLOCK
    
    if (!awgt)
        return;
    
    if (!(awgt->state() & ArenaWidget::Singleton))
        return;
    
    if (awgt->state() & ArenaWidget::Hidden)
        ArenaWidgetManager::getInstance()->activate(awgt);
    else
        ArenaWidgetManager::getInstance()->rem(awgt);
}

