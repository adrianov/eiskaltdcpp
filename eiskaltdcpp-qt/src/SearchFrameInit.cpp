/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SearchFrame.h"
#include "SearchFramePrivate.h"
#include "SearchFrameLocal.h"
#include "SearchFileTypes.h"
#include "SearchModel.h"
#include "SearchProxyModel.h"
#include "WulforUtil.h"
#include "WulforSettings.h"
#include "MainWindow.h"
#include "GlobalTimer.h"

#include "dcpp/SettingsManager.h"
#include "dcpp/SearchManager.h"

#include <QHeaderView>
#include <QShortcut>
#include <QTimer>
#include <QCompleter>
#include <QMenu>

using namespace dcpp;

void SearchFrame::init(){
    Q_D(SearchFrame);

    d->model = new SearchModel(nullptr);
    d->str_model = new SearchStringListModel(this);
    d->proxy = new SearchProxyModel(this);
    d->proxy->setSourceModel(d->model);

    frame_FILTER->setVisible(false);

    pushButton_STOP->hide();

    treeView_RESULTS->setModel(d->proxy);
    treeView_RESULTS->setContextMenuPolicy(Qt::CustomContextMenu);
    treeView_RESULTS->header()->setContextMenuPolicy(Qt::CustomContextMenu);

    treeView_HUBS->setModel(d->str_model);

    d->arena_menu = new QMenu(this->windowTitle());
    QAction *close_wnd = new QAction(WICON(WulforUtil::eiFILECLOSE), tr("Close"), d->arena_menu);
    d->arena_menu->addAction(close_wnd);
    SearchFileTypes::fillCombo(comboBox_FILETYPES);

    QString     raw  = QByteArray::fromBase64(WSGET(WS_SEARCH_HISTORY).toUtf8());
    d->searchHistory = raw.replace("\r","").split('\n', WULFOR_SKIP_EMPTY);

    QMenu *m = new QMenu();

    for (const auto &s : d->searchHistory)
        m->addAction(s);

    d->focusShortcut = new QShortcut(QKeySequence(Qt::Key_F6), this);
    d->focusShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    lineEdit_SEARCHSTR->setMenu(m);
    lineEdit_SEARCHSTR->setPixmap(WICON_SIZE(WulforUtil::eiEDITADD, 16));

    connect(this, SIGNAL(coreClientConnected(QString)),    this, SLOT(onHubAdded(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreClientDisconnected(QString)), this, SLOT(onHubRemoved(QString)),Qt::QueuedConnection);
    connect(this, SIGNAL(coreClientUpdated(QString)),      this, SLOT(onHubChanged(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreSR(VarMap)),                  this, SLOT(queueResult(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreDownloadFinished(QString)),   this, SLOT(slotDownloadFinished(QString)), Qt::QueuedConnection);

    d->resultFlush = new QTimer(this);
    d->resultFlush->setSingleShot(true);
    d->resultFlush->setInterval(50);
    connect(d->resultFlush, SIGNAL(timeout()), this, SLOT(flushResults()));

    connect(d->focusShortcut, SIGNAL(activated()), lineEdit_SEARCHSTR, SLOT(setFocus()));
    connect(d->focusShortcut, SIGNAL(activated()), lineEdit_SEARCHSTR, SLOT(selectAll()));
    connect(close_wnd, SIGNAL(triggered()), this, SLOT(slotClose()));
    connect(pushButton_SEARCH, SIGNAL(clicked()), this, SLOT(slotStartSearch()));
    connect(pushButton_STOP, SIGNAL(clicked()), this, SLOT(slotStopSearch()));
    connect(pushButton_CLEAR, SIGNAL(clicked()), this, SLOT(slotClear()));
    connect(treeView_RESULTS, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(slotResultDoubleClicked(QModelIndex)));
    connect(treeView_RESULTS, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotContextMenu(QPoint)));
    connect(treeView_RESULTS->header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotHeaderMenu(QPoint)));
    connect(GlobalTimer::getInstance(), SIGNAL(second()), this, SLOT(slotTimer()));
    connect(pushButton_SIDEPANEL, SIGNAL(clicked()), this, SLOT(slotToggleSidePanel()));
    connect(lineEdit_SEARCHSTR, SIGNAL(returnPressed()), this, SLOT(slotStartSearch()));
    connect(lineEdit_SEARCHSTR, SIGNAL(textChanged(QString)), this, SLOT(slotApplyViewFilters()));
    connect(lineEdit_SIZE, SIGNAL(textChanged(QString)), this, SLOT(slotApplyViewFilters()));
    connect(comboBox_SIZE, SIGNAL(currentIndexChanged(int)), this, SLOT(slotApplyViewFilters()));
    connect(comboBox_SIZETYPE, SIGNAL(currentIndexChanged(int)), this, SLOT(slotApplyViewFilters()));
    connect(comboBox_FILETYPES, SIGNAL(currentIndexChanged(int)), this, SLOT(slotApplyViewFilters()));

    connect(WulforSettings::getInstance(), SIGNAL(strValueChanged(QString,QString)), this, SLOT(slotSettingsChanged(QString,QString)));

    load();

    applyViewFiltersNow();

    setAttribute(Qt::WA_DeleteOnClose);

    QList<int> panes = splitter->sizes();
    d->left_pane_old_size = panes[0];

    lineEdit_SEARCHSTR->setFocus();

#ifdef USE_QT_SQLITE
    label_INDEX_STATS->setWordWrap(true);
    SearchFrameLocal::refreshIndexStats(this);
#endif
}

QWidget *SearchFrame::getWidget(){
    return this;
}

QString SearchFrame::getArenaTitle(){
    Q_D(SearchFrame);

    return d->arena_title;
}

QString SearchFrame::getArenaShortTitle(){
    return getArenaTitle();
}

QMenu *SearchFrame::getMenu(){
    Q_D(SearchFrame);

    return d->arena_menu;
}

const QPixmap &SearchFrame::getPixmap(){
    return WICON(WulforUtil::eiFILEFIND);
}

