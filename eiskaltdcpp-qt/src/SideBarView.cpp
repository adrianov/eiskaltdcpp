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
#include "ArenaWidgetManager.h"
#include "ArenaWidgetFactory.h"
#include "QuickConnect.h"
#include "SearchFrame.h"
#include "ShareBrowser.h"
#include "GlobalTimer.h"

#include <QMenu>
#include <QItemSelectionModel>
#include <QFileDialog>
#include <QDesktopServices>
#include <QDir>
#include <QUrl>
#include <QScrollBar>
#include <QHeaderView>
#include <QEvent>

using namespace dcpp;

SideBarView::SideBarView ( QWidget* parent ) : QTreeView(parent), _model(nullptr) {
    installEventFilter(this);

    _model = new SideBarModel(this);
    
    setModel(_model);
    setItemsExpandable(true);
    setHeaderHidden(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setItemDelegate(new SideBarDelegate(this));
    expandAll();

    connect(ArenaWidgetManager::getInstance(), SIGNAL(activated(ArenaWidget*)), this,   SLOT(activated(ArenaWidget*)));
    connect(ArenaWidgetManager::getInstance(), SIGNAL(added(ArenaWidget*)),     this,   SLOT(added(ArenaWidget*)));
    connect(ArenaWidgetManager::getInstance(), SIGNAL(removed(ArenaWidget*)),   this,   SLOT(removed(ArenaWidget*)));
    connect(ArenaWidgetManager::getInstance(), SIGNAL(activated(ArenaWidget*)), _model, SLOT(mapped(ArenaWidget*)));
    connect(ArenaWidgetManager::getInstance(), SIGNAL(toggled(ArenaWidget*)),   _model, SLOT(toggled(ArenaWidget*)));
    connect(ArenaWidgetManager::getInstance(), SIGNAL(updated(ArenaWidget*)),   _model, SLOT(updated(ArenaWidget*)));
    
    connect(this, SIGNAL(doubleClicked(QModelIndex)),           this,   SLOT(slotSideBarDblClicked(QModelIndex)));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)),   this,   SLOT(slotSidebarContextMenu()));
    connect(this, SIGNAL(clicked(QModelIndex)),                 this,   SLOT(slotSidebarHook(QModelIndex)));
    connect(this, SIGNAL(clicked(QModelIndex)),                 _model, SLOT(slotIndexClicked(QModelIndex)));

    connect(GlobalTimer::getInstance(), SIGNAL(second()), _model, SLOT(redraw()));

    connect(horizontalScrollBar(), SIGNAL(rangeChanged(int,int)), this, SLOT(slotUpdateHeaderSize()));
    connect(verticalScrollBar(), SIGNAL(rangeChanged(int,int)), this, SLOT(slotUpdateHeaderSize()));

    connect(_model,    SIGNAL(mapWidget(ArenaWidget*)),     ArenaWidgetManager::getInstance(),  SLOT(activate(ArenaWidget*)));
    connect(_model,    SIGNAL(selectIndex(QModelIndex)),    this,                               SLOT(slotWidgetActivated(QModelIndex)));
}

SideBarView::~SideBarView() {

}

bool SideBarView::eventFilter ( QObject *obj, QEvent *e) {
    if (obj == this && e->type() == QEvent::Resize) {
        slotUpdateHeaderSize();
    }
    
    return QObject::eventFilter (obj , e);
}

void SideBarView::activated ( ArenaWidget *awgt ) {
    _model->mapped(awgt);
}

void SideBarView::added ( ArenaWidget *awgt ) {
    _model->insertWidget(awgt);
}

void SideBarView::removed ( ArenaWidget *awgt ) {
    _model->removeWidget(awgt);
}

