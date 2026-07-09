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
#include "WulforUtil.h"
#include "ArenaWidgetManager.h"
#include "DebugHelper.h"

void TabFrame::removeWidget(ArenaWidget *awgt){
    DEBUG_BLOCK
    
    if (!awgt_map.contains(awgt))
        return;

    TabButton *btn = const_cast<TabButton*>(awgt_map.value(awgt));

    fr_layout->removeWidget(btn);
    tbtn_map.remove(btn);
    awgt_map.remove(awgt);
    tabIconKeys.remove(awgt);

    btn->deleteLater();

    historyPurge(awgt);
    historyPop();
    
     if (awgt->toolButton())
        awgt->toolButton()->setChecked(false);
}

void TabFrame::insertWidget(ArenaWidget *awgt){
    DEBUG_BLOCK
    
    if (awgt_map.contains(awgt) || (awgt && (awgt->state() & ArenaWidget::Hidden)) || !awgt)
        return;

    TabButton *btn = new TabButton();
    btn->setText(awgt->getArenaShortTitle().left(32));
    btn->setToolTip(WulforUtil::getInstance()->compactToolTipText(awgt->getArenaTitle(), 60, "\n"));
    btn->setWidgetIcon(awgt->getPixmap());
    btn->setContextMenuPolicy(Qt::CustomContextMenu);
    btn->installEventFilter(this);

    fr_layout->addWidget(btn);

    awgt_map.insert(awgt, btn);
    tbtn_map.insert(btn, awgt);
    tabIconKeys.insert(awgt, awgt->getPixmap().cacheKey());
    
    if (awgt->toolButton())
        awgt->toolButton()->setChecked(true);

    connect(btn, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotContextMenu()));
    connect(btn, SIGNAL(clicked()), this, SLOT(buttonClicked()));
    connect(btn, SIGNAL(closeRequest()), this, SLOT(closeRequsted()));
    connect(btn, SIGNAL(dropped(TabButton*)), this, SLOT(slotDropped(TabButton*)));
}

bool TabFrame::hasWidget(ArenaWidget *awgt) const{
    DEBUG_BLOCK
    
    return awgt_map.contains(awgt);
}

void TabFrame::mapped(ArenaWidget *awgt){
    DEBUG_BLOCK
    
    if (!awgt_map.contains(awgt))
        return;

    TabButton *btn = const_cast<TabButton*>(awgt_map.value(awgt));

    btn->setChecked(true);
    btn->setFocus();

    historyPush(awgt);
}

void TabFrame::updated ( ArenaWidget* awgt ) {
    DEBUG_BLOCK
    
    if (awgt->state() & ArenaWidget::Hidden){
        removeWidget(awgt);
    }
    else if (!awgt_map.contains(awgt)){
        insertWidget(awgt);
    }
}

void TabFrame::redraw() {
    DEBUG_BLOCK
    
    for (auto it = tbtn_map.begin(); it != tbtn_map.end(); ++it){
        TabButton *btn = const_cast<TabButton*>(it.key());
        ArenaWidget *awgt = const_cast<ArenaWidget*>(it.value());

        const QString text = awgt->getArenaShortTitle().left(32);
        const QString tip = WulforUtil::getInstance()->compactToolTipText(awgt->getArenaTitle(), 60, "\n");
        const quint64 iconKey = awgt->getPixmap().cacheKey();
        if (btn->text() != text)
            btn->setText(text);
        if (btn->toolTip() != tip)
            btn->setToolTip(tip);
        if (tabIconKeys.value(awgt) != iconKey) {
            btn->setWidgetIcon(awgt->getPixmap());
            tabIconKeys[awgt] = iconKey;
        }

        if (awgt->state() & ArenaWidget::Hidden)
            continue;
        else
            btn->resetGeometry();
    }
}

