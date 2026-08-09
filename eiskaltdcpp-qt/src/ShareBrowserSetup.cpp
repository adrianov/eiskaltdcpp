/***************************************************************************
*                                                                         *
*   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
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
#include "MediaEnrichQueue.h"
#include "filebrowser/FileBrowserFilterProxy.h"
#include "filebrowser/ListFilterProxy.h"
#include "SearchFileTypes.h"
#include "MainWindow.h"
#include "ArenaWidgetManager.h"
#include "AutoToolTip.h"
#include "sharebrowser/ListingMediaIndex.h"

#include "dcpp/ADLSearch.h"
#include "dcpp/ClientManager.h"

#include <QHeaderView>
#include <QAction>

using namespace dcpp;

namespace {

void hideTreeExtraColumns(QHeaderView *h)
{
    if (!h)
        return;
    h->hideSection(COLUMN_FILEBROWSER_ESIZE);
    h->hideSection(COLUMN_FILEBROWSER_TTH);
    h->hideSection(COLUMN_FILEBROWSER_BR);
    h->hideSection(COLUMN_FILEBROWSER_WH);
    h->hideSection(COLUMN_FILEBROWSER_MVIDEO);
    h->hideSection(COLUMN_FILEBROWSER_MAUDIO);
    h->hideSection(COLUMN_FILEBROWSER_HIT);
    h->hideSection(COLUMN_FILEBROWSER_TS);
    h->hideSection(COLUMN_FILEBROWSER_PATH);
}

} // namespace

void ShareBrowser::init(){
    setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    toolButton_UP->setIcon(WICON(AppIcons::eiTOP));
    toolButton_FORWARD->setIcon(WICON(AppIcons::eiNEXT));
    toolButton_BACK->setIcon(WICON(AppIcons::eiPREVIOUS));

    initModels();

    SearchFileTypes::fillCombo(comboBox_FILETYPES);
    lineEdit_FILTER->setPlaceholderText(tr("Filter path (space-separated, -exclude)"));
    lineEdit_FILTER->setToolTip(tr("Filter by path/name. Space-separated terms; prefix - to exclude."));

    proxy = new ListFilterProxy(this);
    proxy->setSourceModel(list_model);

    tree_proxy = new FileBrowserFilterProxy(this);
    tree_proxy->setSourceModel(tree_model);

    lineEdit_FILTER->installEventFilter(this);

    treeView_LPANE->setModel(tree_proxy);
    hideTreeExtraColumns(treeView_LPANE->header());

    treeView_LPANE->setExpanded(treeMapFromSource(tree_model->index(0, 0)), true);
    treeView_LPANE->setContextMenuPolicy(Qt::CustomContextMenu);

    treeView_RPANE->setModel(proxy);
    treeView_RPANE->setUniformRowHeights(true);
    treeView_RPANE->setContextMenuPolicy(Qt::CustomContextMenu);
    treeView_RPANE->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    treeView_RPANE->installEventFilter(this);

    AutoToolTipDelegate *rpaneTip = new AutoToolTipDelegate(treeView_RPANE);
    rpaneTip->setElideLeftColumns({COLUMN_FILEBROWSER_PATH});
    treeView_RPANE->setItemDelegate(rpaneTip);

    arena_menu = new QMenu(tr("Filebrowser"));

    QAction *add_fav = new QAction(WICON(AppIcons::eiFAVADD), tr("Add User to Favorites"), arena_menu);
    add_fav->setEnabled(user && user != ClientManager::getInstance()->getMe());
    QAction *close_wnd = new QAction(WICON(AppIcons::eiFILECLOSE), tr("Close"), arena_menu);
    arena_menu->addAction(add_fav);
    arena_menu->addSeparator();
    arena_menu->addAction(close_wnd);

    connect(treeView_LPANE, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotCustomContextMenu(QPoint)));
    connect(treeView_LPANE->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(slotLeftPaneSelChanged(QItemSelection,QItemSelection)));

    connect(add_fav, SIGNAL(triggered()), this, SLOT(slotAddToFavorites()));
    connect(close_wnd, SIGNAL(triggered()), this, SLOT(slotClose()));

    connect(lineEdit_FILTER, SIGNAL(textChanged(QString)), this, SLOT(slotApplyFilters()));
    connect(lineEdit_SIZE, SIGNAL(textChanged(QString)), this, SLOT(slotApplyFilters()));
    connect(comboBox_SIZE, SIGNAL(currentIndexChanged(int)), this, SLOT(slotApplyFilters()));
    connect(comboBox_SIZETYPE, SIGNAL(currentIndexChanged(int)), this, SLOT(slotApplyFilters()));
    connect(comboBox_FILETYPES, SIGNAL(currentIndexChanged(int)), this, SLOT(slotApplyFilters()));
    connect(pushButton_CLEAR, SIGNAL(clicked()), this, SLOT(slotClearFilters()));
    connect(checkBox_FLAT, SIGNAL(toggled(bool)), this, SLOT(slotFlatToggled(bool)));

    connect(treeView_RPANE->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(slotRightPaneSelChanged(QItemSelection,QItemSelection)));
    connect(treeView_RPANE, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotCustomContextMenu(QPoint)));
    connect(treeView_RPANE->header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotHeaderMenu()));
    connect(treeView_RPANE, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(slotRightPaneClicked(QModelIndex)));
    connect(tree_model, SIGNAL(layoutChanged()), this, SLOT(slotLayoutUpdated()));
    connect(WulforSettings::getInstance(), SIGNAL(strValueChanged(QString,QString)), this, SLOT(slotSettingsChanged(QString,QString)));
    connect(toolButton_BACK, SIGNAL(clicked()), this, SLOT(slotButtonBack()));
    connect(toolButton_FORWARD, SIGNAL(clicked()), this, SLOT(slotButtonForward()));
    connect(toolButton_UP, SIGNAL(clicked()), this, SLOT(slotButtonUp()));

    applyViewFiltersNow();

    continueInit();
}

void ShareBrowser::load(){
    restoreSplitterSizes();

    WulforUtil::restoreTreeHeader(treeView_LPANE->header(), QByteArray::fromBase64(WSGET(WS_SHARE_LPANE_STATE).toUtf8()));
    WulforUtil::restoreTreeHeader(treeView_RPANE->header(), QByteArray::fromBase64(WSGET(WS_SHARE_RPANE_STATE).toUtf8()));

    hideTreeExtraColumns(treeView_LPANE->header());
    if (!checkBox_FLAT->isChecked())
        treeView_RPANE->header()->hideSection(COLUMN_FILEBROWSER_PATH);

    treeView_LPANE->setSortingEnabled(true);
    treeView_RPANE->setSortingEnabled(true);
}

void ShareBrowser::save(){
    WSSET(WS_SHARE_LPANE_STATE, treeView_LPANE->header()->saveState().toBase64());
    WSSET(WS_SHARE_RPANE_STATE, treeView_RPANE->header()->saveState().toBase64());
    WBSET(WB_SHARE_FLAT, checkBox_FLAT->isChecked());

    if (!flatMode) {
        WISET(WI_SHARE_RPANE_WIDTH, treeView_RPANE->width());
        WISET(WI_SHARE_WIDTH, treeView_RPANE->width() + treeView_LPANE->width());
    }
}

void ShareBrowser::buildList(){
    try {
        listing.loadFile(file.toStdString());
    } catch (const Exception &e) {
        emit die(tr("Share browser error: %1").arg(_q(e.what())));
        return;
    }
    try {
        listing.getRoot()->setName(nick.toStdString());
        ADLSearchManager::getInstance()->matchListing(listing);
    } catch (const Exception &e) {
        emit die(tr("Share browser error: %1").arg(_q(e.what())));
        return;
    }

    ListingMediaIndex mediaIndex;
    mediaIndex.collectFrom(listing.getRoot());
    mediaIndex.publish(user, file, nick);
}

void ShareBrowser::initModels(){
    tree_model = new FileBrowserModel();
    tree_model->setListing(&listing);
    tree_model->fetchMore(QModelIndex());
    tree_root  = tree_model->getRootElem();

    list_model = new FileBrowserModel();
    list_root = list_model->getRootElem();
    folderList = new ShareFolderList(list_model, list_root);

    mediaEnrich = new MediaEnrichQueue(this);
    connect(mediaEnrich, SIGNAL(ready(QVariant)), this, SLOT(applyMediaEnrich(QVariant)));
}
