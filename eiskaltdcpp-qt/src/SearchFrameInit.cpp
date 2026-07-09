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
#include "SearchModel.h"
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

    for (int i = 0; i < d->model->columnCount(); i++)
        comboBox_FILTERCOLUMNS->addItem(d->model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString());

    comboBox_FILTERCOLUMNS->setCurrentIndex(COLUMN_SF_FILENAME);

    frame_FILTER->setVisible(false);

    pushButton_STOP->hide();

    toolButton_CLOSEFILTER->setIcon(WICON(WulforUtil::eiEDITDELETE));

    treeView_RESULTS->setModel(d->model);
    treeView_RESULTS->setContextMenuPolicy(Qt::CustomContextMenu);
    treeView_RESULTS->header()->setContextMenuPolicy(Qt::CustomContextMenu);

    treeView_HUBS->setModel(d->str_model);

    d->arena_menu = new QMenu(this->windowTitle());
    QAction *close_wnd = new QAction(WICON(WulforUtil::eiFILECLOSE), tr("Close"), d->arena_menu);
    d->arena_menu->addAction(close_wnd);
    const SettingsManager::SearchTypes &searchTypes = SettingsManager::getInstance()->getSearchTypes();
    QStringList filetypes;
    // Predefined
    for (int i = SearchManager::TYPE_ANY; i < SearchManager::TYPE_LAST; i++)
    {
            filetypes << _q(SearchManager::getTypeStr(i));
    }

    // Customs
    for (const auto &i : searchTypes)
    {
        string type = i.first;
        if (!(type.size() == 1 && type[0] >= '1' && type[0] <= '7'))
        {
                filetypes << _q(type);
        }
    }
    comboBox_FILETYPES->addItems(filetypes);
    comboBox_FILETYPES->setCurrentIndex(0);

    QList<WulforUtil::Icons> icons;
    icons   << WulforUtil::eiFILETYPE_UNKNOWN  << WulforUtil::eiFILETYPE_MP3         << WulforUtil::eiFILETYPE_ARCHIVE
            << WulforUtil::eiFILETYPE_DOCUMENT << WulforUtil::eiFILETYPE_APPLICATION << WulforUtil::eiFILETYPE_PICTURE
            << WulforUtil::eiFILETYPE_VIDEO    << WulforUtil::eiFOLDER_BLUE          << WulforUtil::eiFIND
            << WulforUtil::eiFILETYPE_ARCHIVE;

    for (int i = 0; i < icons.size(); i++)
        comboBox_FILETYPES->setItemIcon(i, WICON(icons.at(i)));

    QString     raw  = QByteArray::fromBase64(WSGET(WS_SEARCH_HISTORY).toUtf8());
    d->searchHistory = raw.replace("\r","").split('\n', WULFOR_SKIP_EMPTY);

    QMenu *m = new QMenu();

    for (const auto &s : d->searchHistory)
        m->addAction(s);

    d->focusShortcut = new QShortcut(QKeySequence(Qt::Key_F6), this);
    d->focusShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    lineEdit_SEARCHSTR->setMenu(m);
    lineEdit_SEARCHSTR->setPixmap(WICON_SIZE(WulforUtil::eiEDITADD, 16));

    lineEdit_FILTER->installEventFilter(this);

    connect(this, SIGNAL(coreClientConnected(QString)),    this, SLOT(onHubAdded(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreClientDisconnected(QString)), this, SLOT(onHubRemoved(QString)),Qt::QueuedConnection);
    connect(this, SIGNAL(coreClientUpdated(QString)),      this, SLOT(onHubChanged(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreSR(VarMap)),                  this, SLOT(addResult(VarMap)), Qt::QueuedConnection);

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
    connect(lineEdit_SIZE,      SIGNAL(returnPressed()), this, SLOT(slotStartSearch()));
    connect(comboBox_FILETYPES, SIGNAL(currentIndexChanged(int)), lineEdit_SEARCHSTR, SLOT(setFocus()));
    connect(comboBox_FILETYPES, SIGNAL(currentIndexChanged(int)), lineEdit_SEARCHSTR, SLOT(selectAll()));
    connect(toolButton_CLOSEFILTER, SIGNAL(clicked()), this, SLOT(slotFilter()));
    connect(comboBox_FILTERCOLUMNS, SIGNAL(currentIndexChanged(int)), lineEdit_FILTER, SLOT(selectAll()));
    connect(comboBox_FILTERCOLUMNS, SIGNAL(currentIndexChanged(int)), this, SLOT(slotChangeProxyColumn(int)));

    connect(WulforSettings::getInstance(), SIGNAL(strValueChanged(QString,QString)), this, SLOT(slotSettingsChanged(QString,QString)));

    load();

    setAttribute(Qt::WA_DeleteOnClose);

    QList<int> panes = splitter->sizes();
    d->left_pane_old_size = panes[0];

    lineEdit_SEARCHSTR->setFocus();
}

void SearchFrame::initSecond(){
    /*Q_D(SearchFrame);

    if (!d->timer){
        d->timer = new QTimer(this);
        d->timer->setInterval(1000);
        d->timer->setSingleShot(true);

        connect(d->timer, SIGNAL(timeout()), this, SLOT(timerTick()));
    }

    d->timer->start();*/
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

