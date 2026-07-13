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
#include "WulforSettings.h"
#include "FileBrowserModel.h"
#include "MainWindow.h"
#include "ArenaWidgetManager.h"

#include "dcpp/ClientManager.h"
#include "dcpp/ADLSearch.h"

#include <QHeaderView>
#include <QAction>

using namespace dcpp;

void ShareBrowser::init(){
    setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    toolButton_UP->setIcon(WICON(WulforUtil::eiTOP));
    toolButton_FORWARD->setIcon(WICON(WulforUtil::eiNEXT));
    toolButton_BACK->setIcon(WICON(WulforUtil::eiPREVIOUS));

    frame_FILTER->setVisible(false);

    initModels();

    lineEdit_FILTER->installEventFilter(this);

    treeView_LPANE->setModel(tree_model);

    treeView_LPANE->header()->hideSection(COLUMN_FILEBROWSER_ESIZE);
    treeView_LPANE->header()->hideSection(COLUMN_FILEBROWSER_TTH);
    treeView_LPANE->header()->hideSection(COLUMN_FILEBROWSER_BR);
    treeView_LPANE->header()->hideSection(COLUMN_FILEBROWSER_WH);
    treeView_LPANE->header()->hideSection(COLUMN_FILEBROWSER_MVIDEO);
    treeView_LPANE->header()->hideSection(COLUMN_FILEBROWSER_MAUDIO);
    treeView_LPANE->header()->hideSection(COLUMN_FILEBROWSER_HIT);
    treeView_LPANE->header()->hideSection(COLUMN_FILEBROWSER_TS);

    treeView_LPANE->setExpanded(tree_model->index(0, 0), true);
    treeView_LPANE->setContextMenuPolicy(Qt::CustomContextMenu);

    treeView_RPANE->setModel(list_model);
    treeView_RPANE->setContextMenuPolicy(Qt::CustomContextMenu);
    treeView_RPANE->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    treeView_RPANE->installEventFilter(this);

    toolButton_CLOSEFILTER->setIcon(WICON(WulforUtil::eiEDITDELETE));
    toolButton_SEARCH->setIcon(WICON(WulforUtil::eiFIND));
    toolButton_MATCHLIST->setIcon(WICON(WulforUtil::eiDOWNLIST));

    arena_menu = new QMenu(tr("Filebrowser"));

    QAction *add_fav = new QAction(WICON(WulforUtil::eiFAVADD), tr("Add User to Favorites"), arena_menu);
    add_fav->setEnabled(user && user != ClientManager::getInstance()->getMe());
    QAction *close_wnd = new QAction(WICON(WulforUtil::eiFILECLOSE), tr("Close"), arena_menu);
    arena_menu->addAction(add_fav);
    arena_menu->addSeparator();
    arena_menu->addAction(close_wnd);

    connect(treeView_LPANE, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotCustomContextMenu(QPoint)));
    connect(treeView_LPANE->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(slotLeftPaneSelChanged(QItemSelection,QItemSelection)));

    connect(add_fav, SIGNAL(triggered()), this, SLOT(slotAddToFavorites()));
    connect(close_wnd, SIGNAL(triggered()), this, SLOT(slotClose()));
    connect(toolButton_CLOSEFILTER, SIGNAL(clicked()), this, SLOT(slotFilter()));

    connect(treeView_RPANE->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(slotRightPaneSelChanged(QItemSelection,QItemSelection)));
    connect(treeView_RPANE, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotCustomContextMenu(QPoint)));
    connect(treeView_RPANE->header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotHeaderMenu()));
    connect(treeView_RPANE, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(slotRightPaneClicked(QModelIndex)));
    connect(tree_model, SIGNAL(layoutChanged()), this, SLOT(slotLayoutUpdated()));
    connect(WulforSettings::getInstance(), SIGNAL(strValueChanged(QString,QString)), this, SLOT(slotSettingsChanged(QString,QString)));
    connect(toolButton_SEARCH, SIGNAL(clicked()), this, SLOT(slotStartSearch()));
    connect(toolButton_BACK, SIGNAL(clicked()), this, SLOT(slotButtonBack()));
    connect(toolButton_FORWARD, SIGNAL(clicked()), this, SLOT(slotButtonForward()));
    connect(toolButton_UP, SIGNAL(clicked()), this, SLOT(slotButtonUp()));
    connect(toolButton_MATCHLIST, SIGNAL(clicked()), this, SLOT(slotMatchList()));

    continueInit();
}

void ShareBrowser::load(){
    int w = WIGET(WI_SHARE_WIDTH);
    int wr= WIGET(WI_SHARE_RPANE_WIDTH);

    if (w >= 0 && wr >= 0){
        QList<int> frames;

        frames << (w - wr) << wr;

        splitter->setSizes(frames);
    }

    WulforUtil::restoreTreeHeader(treeView_LPANE->header(), QByteArray::fromBase64(WSGET(WS_SHARE_LPANE_STATE).toUtf8()));
    WulforUtil::restoreTreeHeader(treeView_RPANE->header(), QByteArray::fromBase64(WSGET(WS_SHARE_RPANE_STATE).toUtf8()));

    treeView_LPANE->header()->hideSection(COLUMN_FILEBROWSER_ESIZE);
    treeView_LPANE->header()->hideSection(COLUMN_FILEBROWSER_TTH);
    treeView_LPANE->header()->hideSection(COLUMN_FILEBROWSER_BR);
    treeView_LPANE->header()->hideSection(COLUMN_FILEBROWSER_WH);
    treeView_LPANE->header()->hideSection(COLUMN_FILEBROWSER_MVIDEO);
    treeView_LPANE->header()->hideSection(COLUMN_FILEBROWSER_MAUDIO);
    treeView_LPANE->header()->hideSection(COLUMN_FILEBROWSER_HIT);
    treeView_LPANE->header()->hideSection(COLUMN_FILEBROWSER_TS);

    treeView_LPANE->setSortingEnabled(true);
    treeView_RPANE->setSortingEnabled(true);

    itemsCount = listing.getRoot()->getTotalFileCount(true);
    share_size = listing.getRoot()->getTotalSize(true);

    label_LEFT->setText(QString(tr("Total share size: %1;  Files: %2")).arg(WulforUtil::formatBytes(share_size)).arg(itemsCount));
}

void ShareBrowser::save(){
    WSSET(WS_SHARE_LPANE_STATE, treeView_LPANE->header()->saveState().toBase64());
    WSSET(WS_SHARE_RPANE_STATE, treeView_RPANE->header()->saveState().toBase64());

    WISET(WI_SHARE_RPANE_WIDTH, treeView_RPANE->width());
    WISET(WI_SHARE_WIDTH, treeView_RPANE->width() + treeView_LPANE->width());
}

void ShareBrowser::buildList(){
    try {
        listing.loadFile(file.toStdString());
        listing.getRoot()->setName(nick.toStdString());
        ADLSearchManager::getInstance()->matchListing(listing);
    }
    catch (const Exception &e){
        emit die(tr("Share browser error: %1").arg(_q(e.what())));
    }
}

void ShareBrowser::initModels(){
    tree_model = new FileBrowserModel();
    tree_model->setListing(&listing);
    tree_model->fetchMore(QModelIndex());
    tree_root  = tree_model->getRootElem();

    list_model = new FileBrowserModel();
    list_root = list_model->getRootElem();
}
