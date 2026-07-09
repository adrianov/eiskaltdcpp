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

#include "VersionGlobal.h"

void ToolBar::redraw(){
    bool changed = false;
    for (auto it = map.begin(); it != map.end(); ++it){
        ArenaWidget *awgt = it.key();
        const int index = it.value();
        const QString text = awgt->getArenaShortTitle().left(32);
        const QString tip = WulforUtil::getInstance()->compactToolTipText(awgt->getArenaTitle(), 60, "\n");
        const quint64 iconKey = awgt->getPixmap().cacheKey();

        if (tabbar->tabText(index) != text) {
            tabbar->setTabText(index, text);
            changed = true;
        }
        if (tabbar->tabToolTip(index) != tip) {
            tabbar->setTabToolTip(index, tip);
            changed = true;
        }
        if (tabIconKeys.value(awgt) != iconKey) {
            tabbar->setTabIcon(index, awgt->getPixmap());
            tabIconKeys[awgt] = iconKey;
            changed = true;
        }
    }

    if (changed)
        tabbar->repaint();

    ArenaWidget *awgt = findWidgetForIndex(tabbar->currentIndex());

    if (awgt) {
        const QString title = awgt->getArenaTitle() +
                " :: " + QString::fromStdString(eiskaltdcppAppNameString);
        if (MainWindow::getInstance()->windowTitle() != title)
            MainWindow::getInstance()->setWindowTitle(title);
    }
}

void ToolBar::nextTab(){
    if (!tabbar)
        return;

    if (tabbar->currentIndex()+1 < tabbar->count())
        tabbar->setCurrentIndex(tabbar->currentIndex()+1);
    else
        tabbar->setCurrentIndex(0);
}

void ToolBar::prevTab(){
    if (!tabbar)
        return;

    if (tabbar->currentIndex()-1 >= 0)
        tabbar->setCurrentIndex(tabbar->currentIndex()-1);
    else
        tabbar->setCurrentIndex(tabbar->count()-1);
}

void ToolBar::rebuildIndexes(const int removed){
    if (removed < 0)
        return;

    for (auto it = map.begin(); it != map.end(); ++it){
        if (it.value() > removed)
            map[it.key()] = it.value()-1;
    }
}

void ToolBar::mapped(ArenaWidget *awgt){
    blockSignals(true);
    if (map.contains(awgt))
        tabbar->setCurrentIndex(map[awgt]);

    redraw();

    blockSignals(false);
}

bool ToolBar::hasWidget(ArenaWidget *w) const{
    return map.contains(w);
}

void ToolBar::mapWidget(ArenaWidget *w){
    if (hasWidget(w))
        tabbar->setCurrentIndex(map[w]);
}
