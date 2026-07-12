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
#include "SearchFrameLocal.h"
#include "WulforUtil.h"
#include "GlobalTimer.h"
#include "ArenaWidgetManager.h"

#include "dcpp/SearchManager.h"
#include "dcpp/ShareManager.h"
#include "dcpp/QueueItem.h"
#include "dcpp/Client.h"
#include "dcpp/Util.h"
#include "dcpp/Text.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QSplitter>

using namespace dcpp;

void SearchFrame::slotFilter(){
    Q_D(SearchFrame);

    if (frame_FILTER->isVisible()){
        treeView_RESULTS->setModel(d->model);

        disconnect(lineEdit_FILTER, SIGNAL(textChanged(QString)), d->proxy, SLOT(setFilterFixedString(QString)));
    }
    else {
        d->proxy = (d->proxy? d->proxy : (new SearchProxyModel(this)));
        d->proxy->setDynamicSortFilter(true);
        d->proxy->setFilterFixedString(lineEdit_FILTER->text());
        d->proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
        d->proxy->setFilterKeyColumn(comboBox_FILTERCOLUMNS->currentIndex());
        d->proxy->setSourceModel(d->model);

        treeView_RESULTS->setModel(d->proxy);

        connect(lineEdit_FILTER, SIGNAL(textChanged(QString)), d->proxy, SLOT(setFilterFixedString(QString)));

        if (!lineEdit_SEARCHSTR->selectedText().isEmpty()){
            lineEdit_FILTER->setText(lineEdit_SEARCHSTR->selectedText());
            lineEdit_FILTER->selectAll();
        }

        lineEdit_FILTER->setFocus();

        if (!lineEdit_FILTER->text().isEmpty())
            lineEdit_FILTER->selectAll();
    }

    frame_FILTER->setVisible(!frame_FILTER->isVisible());
}

void SearchFrame::slotChangeProxyColumn(int col){
    Q_D(SearchFrame);

    if (d->proxy)
        d->proxy->setFilterKeyColumn(col);
}

void SearchFrame::slotSettingsChanged(const QString &key, const QString &value){
    Q_UNUSED(value)
    if (key == WS_TRANSLATION_FILE)
        retranslateUi(this);
}

void SearchFrame::on(SearchManagerListener::SR, const dcpp::SearchResultPtr& aResult) noexcept {
    Q_D(SearchFrame);

    if (d->currentSearch.empty() || !aResult || d->stop == true)
        return;

    // Dropped = does not satisfy this search query (DC++ / Flylink meaning).
    if (!aResult->getToken().empty() && d->token != _q(aResult->getToken())){
        d->dropped++;
        return;
    }

    if(d->isHash) {
        if(aResult->getType() != SearchResult::TYPE_FILE || TTHValue(Text::fromT(d->currentSearch[0])) != aResult->getTTH()) {
            d->dropped++;
            return;
        }
    }
    else {
        for (const auto &j : d->currentSearch) {
            if((*j.begin() != ('-') && Util::findSubString(aResult->getFile(), j) == string::npos) ||
               (*j.begin() == ('-') && j.size() != 1 && Util::findSubString(aResult->getFile(), j.substr(1)) != string::npos)
              )
           {
                    d->dropped++;
                    return;
           }
        }
    }

    // Query matched: keep for ShareIndex even if UI filters hide the row.
    QVariantMap map;
    getParams(map, aResult);
    SearchFrameLocal::upsertHubResult(map);

    // Filtered = matches query but hidden by display preferences (not "bad" results).
    if (d->filterShared == Filter && aResult->getType() == SearchResult::TYPE_FILE){
        if (ShareManager::getInstance()->isTTHShared(aResult->getTTH())) {
            d->filtered++;
            return;
        }
    }

    if (d->withFreeSlots && !aResult->getFreeSlots()){
        d->filtered++;
        return;
    }

    emit coreSR(map);
}

void SearchFrame::slotClose() {
    ArenaWidgetManager::getInstance()->rem(this);
}

void SearchFrame::on(ClientConnected, Client* c) noexcept{
    emit coreClientConnected(_q(c->getHubUrl()));
}

void SearchFrame::on(ClientUpdated, Client* c) noexcept{
    emit coreClientUpdated((_q(c->getHubUrl())));
}

void SearchFrame::on(ClientDisconnected, Client* c) noexcept{
    emit coreClientDisconnected((_q(c->getHubUrl())));
}

void SearchFrame::slotStopSearch(){
    Q_D(SearchFrame);

    d->stop = true;
    d->waitingResults = false;
}

void SearchFrame::on(QueueManagerListener::Finished, QueueItem *qi, const string&, int64_t) noexcept {
    if (!qi || qi->isSet(QueueItem::FLAG_USER_LIST))
        return;

    emit coreDownloadFinished(_q(qi->getTTH().toBase32()));
}

void SearchFrame::on(QueueManagerListener::FileMoved, const string&) noexcept {
    emit coreDownloadFinished(QString());
}

void SearchFrame::slotDownloadFinished(const QString &tth){
    Q_D(SearchFrame);

    if (!d->model)
        return;

    if (tth.isEmpty()) {
        // FileMoved has no TTH; coalesce already-queued empties into one refresh.
        if (d->localRefreshPending)
            return;
        d->localRefreshPending = true;
        QMetaObject::invokeMethod(this, "slotLocalRefreshAll", Qt::QueuedConnection);
        return;
    }

    d->model->refreshLocal(tth);
}

void SearchFrame::slotLocalRefreshAll(){
    Q_D(SearchFrame);

    d->localRefreshPending = false;
    if (d->model)
        d->model->refreshLocal(QString());
}
