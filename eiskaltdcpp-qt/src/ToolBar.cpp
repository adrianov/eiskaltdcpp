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

#include <QMenu>
#include <QMouseEvent>

#include "dcpp/version.h"
#include "ArenaWidget.h"
#include "ArenaWidgetManager.h"
#include "MainWindow.h"
#include "PMWindow.h"
#include "WulforSettings.h"
#include "GlobalTimer.h"

ToolBar::ToolBar(QWidget *parent):
    QToolBar(parent),
    tabbar(nullptr)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
}

ToolBar::~ToolBar(){
    for (const auto &s : shortcuts)
        s->deleteLater();
}

bool ToolBar::eventFilter(QObject *obj, QEvent *e){
    if (e->type() == QEvent::MouseButtonRelease){
        QMouseEvent *m_e = reinterpret_cast<QMouseEvent*>(e);

        if (reinterpret_cast<QTabBar*>(obj) == tabbar && m_e->button() == Qt::MidButton){
            QPoint p = tabbar->mapFromGlobal(QCursor::pos());
            int index = tabbar->tabAt(p);

            if (index >= 0)
                slotClose(index);
        }
    }
    else if (e->type() == QEvent::DragEnter && reinterpret_cast<QTabBar*>(obj) == tabbar) {
        QDragEnterEvent *m_e = reinterpret_cast<QDragEnterEvent*>(e);
        m_e->acceptProposedAction();

        int tab = tabbar->tabAt(m_e->pos());
        if (tab >=0 && tab != tabbar->currentIndex())
            slotIndexChanged(tab);

        return true;
    }
    else if (e->type() == QEvent::DragMove && reinterpret_cast<QTabBar*>(obj) == tabbar) {
        QDragMoveEvent *m_e = reinterpret_cast<QDragMoveEvent*>(e);

        int tab = tabbar->tabAt(m_e->pos());
        if (tab >=0) {
            m_e->acceptProposedAction();
            if (tab != tabbar->currentIndex())
                slotIndexChanged(tab);
        } else {
            m_e->setDropAction(Qt::IgnoreAction);
        }
        return true;
    }

    return QToolBar::eventFilter(obj, e);
}

void ToolBar::showEvent(QShowEvent *e){
    e->accept();

    if (tabbar && e->spontaneous()){
        tabbar->hide();// I know, this is crap, but tabbar->repaint() doesn't fit all tabs in tabbar properly when
        tabbar->show();// MainWindow becomes visible (restoring from system tray)
    }
}

void ToolBar::initTabs(){
    tabbar = new QTabBar(parentWidget());
    tabbar->setObjectName("arenaTabbar");
    tabbar->setTabsClosable(WBGET(WB_APP_TBAR_SHOW_CL_BTNS));
    tabbar->setDocumentMode(true);
    tabbar->setMovable(true);
    tabbar->setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    tabbar->setExpanding(false);
    tabbar->setContextMenuPolicy(Qt::CustomContextMenu);
    tabbar->setSizePolicy(QSizePolicy::Expanding, tabbar->sizePolicy().verticalPolicy());
    tabbar->setAcceptDrops(true);

    tabbar->installEventFilter(this);

    shortcuts << (new QShortcut(QKeySequence(Qt::ALT + Qt::Key_1), parentWidget()))
              << (new QShortcut(QKeySequence(Qt::ALT + Qt::Key_2), parentWidget()))
              << (new QShortcut(QKeySequence(Qt::ALT + Qt::Key_3), parentWidget()))
              << (new QShortcut(QKeySequence(Qt::ALT + Qt::Key_4), parentWidget()))
              << (new QShortcut(QKeySequence(Qt::ALT + Qt::Key_5), parentWidget()))
              << (new QShortcut(QKeySequence(Qt::ALT + Qt::Key_6), parentWidget()))
              << (new QShortcut(QKeySequence(Qt::ALT + Qt::Key_7), parentWidget()))
              << (new QShortcut(QKeySequence(Qt::ALT + Qt::Key_8), parentWidget()))
              << (new QShortcut(QKeySequence(Qt::ALT + Qt::Key_9), parentWidget()))
              << (new QShortcut(QKeySequence(Qt::ALT + Qt::Key_0), parentWidget()));

    for (const auto &s : shortcuts){
        s->setContext(Qt::ApplicationShortcut);

        connect(s, SIGNAL(activated()), this, SLOT(slotShorcuts()));
    }

    connect(tabbar, SIGNAL(currentChanged(int)), this, SLOT(slotIndexChanged(int)));
    connect(tabbar, SIGNAL(tabMoved(int,int)), this, SLOT(slotTabMoved(int,int)));
    connect(tabbar, SIGNAL(tabCloseRequested(int)), this, SLOT(slotClose(int)));
    connect(tabbar, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotContextMenu(QPoint)));
    
    connect(ArenaWidgetManager::getInstance(), SIGNAL(added(ArenaWidget*)),     this, SLOT(insertWidget(ArenaWidget*)));
    connect(ArenaWidgetManager::getInstance(), SIGNAL(removed(ArenaWidget*)),   this, SLOT(removeWidget(ArenaWidget*)));
    connect(ArenaWidgetManager::getInstance(), SIGNAL(activated(ArenaWidget*)), this, SLOT(mapped(ArenaWidget*)));
    connect(ArenaWidgetManager::getInstance(), SIGNAL(updated(ArenaWidget*)),   this, SLOT(updated(ArenaWidget*)));
    connect(ArenaWidgetManager::getInstance(), SIGNAL(toggled(ArenaWidget*)),   this, SLOT(toggled(ArenaWidget*)));
       
    connect(GlobalTimer::getInstance(), SIGNAL(second()), this, SLOT(redraw()));

    addWidget(tabbar);
}

void ToolBar::redraw(){
    for (auto it = map.begin(); it != map.end(); ++it){
        tabbar->setTabText(it.value(), it.key()->getArenaShortTitle().left(32));
        tabbar->setTabToolTip(it.value(), WulforUtil::getInstance()->compactToolTipText(it.key()->getArenaTitle(), 60, "\n"));
        tabbar->setTabIcon(it.value(), it.key()->getPixmap());
    }

    tabbar->repaint();

    ArenaWidget *awgt = findWidgetForIndex(tabbar->currentIndex());

    if (awgt)
        MainWindow::getInstance()->setWindowTitle(awgt->getArenaTitle() +
                                   " :: " + QString::fromStdString(eiskaltdcppAppNameString));
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
