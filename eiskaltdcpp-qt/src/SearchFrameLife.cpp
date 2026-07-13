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
#include "SearchBlacklist.h"
#include "WulforUtil.h"
#include "WulforSettings.h"
#include "MainWindow.h"

#include "dcpp/ClientManager.h"
#include "dcpp/SearchManager.h"
#include "dcpp/QueueManager.h"
#include "dcpp/SettingsManager.h"

#include <QHeaderView>
#include <QShortcut>
#include <QCompleter>
#include <QMenu>

using namespace dcpp;

SearchFrame::SearchFrame(QWidget *parent): QWidget(parent), d_ptr(new SearchFramePrivate())
{
    if (!SearchBlacklist::getInstance())
        SearchBlacklist::newInstance();

    Q_D(SearchFrame);

    d->isHash = false;
    d->arena_menu = nullptr;
    d->dropped = 0L;
    d->filtered = 0L;
    d->results = 0L;
    d->filterShared = SearchFrame::None;
    d->withFreeSlots = false;
    d->saveFileType = true;
    d->proxy = nullptr;
    d->completer = nullptr;
    d->stop = false;
    d->arena_title = tr("Search");
    d->searchStartTime = 0;
    d->searchEndTime = 1;
    d->waitingResults = false;

    setupUi(this);

    init();

    ClientManager* clientMgr = ClientManager::getInstance();

    auto lock = clientMgr->lock();
    clientMgr->addListener(this);
    Client::List& clients = clientMgr->getClients();

    for (const auto &client : clients) {
        if(!client->isConnected())
            continue;

        d->hubs.push_back(_q(client->getHubUrl()));
        d->client_list.push_back(client);
    }

    progressBar->show();
    progressIndicator->hide();

    d->str_model->setStringList(d->hubs);


    for (int i = 0; i < d->str_model->rowCount(); i++)
       d-> str_model->setData(d->str_model->index(i, 0), Qt::Checked, Qt::CheckStateRole);

    SearchManager::getInstance()->addListener(this);
    QueueManager::getInstance()->addListener(this);
}

SearchFrame::~SearchFrame(){
    Menu::deleteInstance();

    Q_D(SearchFrame);

    treeView_RESULTS->setModel(nullptr);

    if (d->completer)
        d->completer->deleteLater();

    if (d->proxy) {
        d->proxy->setSourceModel(nullptr);
        d->proxy->deleteLater();
        d->proxy = nullptr;
    }

    d->arena_menu->deleteLater();

    delete d->model;

    delete d_ptr;
}

void SearchFrame::closeEvent(QCloseEvent *e){
    ClientManager::getInstance()->cancelSearch((void*)this);

    SearchManager::getInstance()->removeListener(this);
    ClientManager::getInstance()->removeListener(this);
    QueueManager::getInstance()->removeListener(this);

    save();

    setAttribute(Qt::WA_DeleteOnClose);

    QWidget::disconnect(this, nullptr, this, nullptr);

    e->accept();
}

void SearchFrame::load(){
    Q_D(SearchFrame);

    WulforUtil::restoreTreeHeader(treeView_RESULTS->header(), QByteArray::fromBase64(WSGET(WS_SEARCH_STATE).toUtf8()));
    treeView_RESULTS->setSortingEnabled(true);

    d->filterShared = static_cast<SearchFrame::AlreadySharedAction>(WIGET(WI_SEARCH_SHARED_ACTION));

    comboBox_SHARED->setCurrentIndex(static_cast<int>(d->filterShared));

    checkBox_FILTERSLOTS->setChecked(WBGET(WB_SEARCHFILTER_NOFREE));
    checkBox_HIDEPANEL->setChecked(WBGET(WB_SEARCH_DONTHIDEPANEL));

    comboBox_FILETYPES->setCurrentIndex(WIGET(WI_SEARCH_LAST_TYPE));

    treeView_RESULTS->sortByColumn(WIGET(WI_SEARCH_SORT_COLUMN), WulforUtil::getInstance()->intToSortOrder(WIGET(WI_SEARCH_SORT_ORDER)));

    QString raw = QByteArray::fromBase64(WSGET(WS_SEARCH_HISTORY).toUtf8());
    QStringList list = raw.replace("\r","").split('\n', WULFOR_SKIP_EMPTY);

    d->completer = new QCompleter(list, lineEdit_SEARCHSTR);
    d->completer->setCaseSensitivity(Qt::CaseInsensitive);
    d->completer->setWrapAround(false);

    lineEdit_SEARCHSTR->setCompleter(d->completer);
}

void SearchFrame::save(){
    Q_D(SearchFrame);

    WSSET(WS_SEARCH_STATE, treeView_RESULTS->header()->saveState().toBase64());
    WISET(WI_SEARCH_SORT_COLUMN, d->model->getSortColumn());
    WISET(WI_SEARCH_SORT_ORDER, WulforUtil::getInstance()->sortOrderToInt(d->model->getSortOrder()));
    WISET(WI_SEARCH_SHARED_ACTION, static_cast<int>(d->filterShared));

    if (d->saveFileType)
        WISET(WI_SEARCH_LAST_TYPE, comboBox_FILETYPES->currentIndex());

    WBSET(WB_SEARCHFILTER_NOFREE, checkBox_FILTERSLOTS->isChecked());
    WBSET(WB_SEARCH_DONTHIDEPANEL, checkBox_HIDEPANEL->isChecked());
}

