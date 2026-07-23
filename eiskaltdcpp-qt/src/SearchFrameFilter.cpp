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
#include "SearchProxyModel.h"
#include "SearchFileTypes.h"
#include "SearchFrameLocal.h"
#include "ShareIndex.h"
#include "WulforUtil.h"
#include "WulforSettings.h"
#include "ArenaWidgetManager.h"

#include "dcpp/SearchManager.h"
#include "dcpp/ClientManager.h"
#include "dcpp/ShareManager.h"
#include "dcpp/Client.h"
#include "dcpp/Util.h"
#include "dcpp/Text.h"

using namespace dcpp;

void SearchFrame::readSizeFilter(quint64 &size, int &mode) const {
    const QString str_size = lineEdit_SIZE->text();
    double lsize = Util::toDouble(Text::fromT(str_size.toStdString()));

    switch (comboBox_SIZE->currentIndex()){
        case 1: lsize *= 1024.0; break;
        case 2: lsize *= (1024.0*1024.0); break;
        case 3: lsize *= (1024.0*1024.0*1024.0); break;
    }

    size = static_cast<quint64>(lsize);
    mode = comboBox_SIZETYPE->currentIndex();
    if (!size || str_size.isEmpty())
        mode = SearchManager::SIZE_DONTCARE;
}

void SearchFrame::slotApplyViewFilters(){
    Q_D(SearchFrame);
    if (d->viewFilterPending)
        return;
    d->viewFilterPending = true;
    QMetaObject::invokeMethod(this, "flushViewFilters", Qt::QueuedConnection);
}

void SearchFrame::flushViewFilters(){
    Q_D(SearchFrame);
    // Cleared by applyViewFiltersNow() callers (e.g. Search start) to cancel this flush.
    if (!d->viewFilterPending)
        return;
    d->viewFilterPending = false;
    applyViewFiltersNow();
}

void SearchFrame::applyViewFiltersNow(){
    Q_D(SearchFrame);
    if (!d->proxy)
        return;

    const QStringList terms =
            lineEdit_SEARCHSTR->text().trimmed().split(QLatin1Char(' '), WULFOR_SKIP_EMPTY);

    quint64 llsize = 0;
    int sizeMode = SearchManager::SIZE_DONTCARE;
    readSizeFilter(llsize, sizeMode);

    const int idx = comboBox_FILETYPES->currentIndex();
    const int typeId = (idx >= 0) ? comboBox_FILETYPES->itemData(idx).toInt() : SearchManager::TYPE_ANY;
    const QString typeName = (idx >= 0) ? comboBox_FILETYPES->itemText(idx) : QString();

    bool dirsOnly = false;
    bool filesOnly = false;
    QStringList exts;
    if (typeId == SearchManager::TYPE_DIRECTORY) {
        dirsOnly = true;
    } else if (typeId != SearchManager::TYPE_ANY) {
        // Match search-start semantics: anything but Any/Directory is files-only.
        filesOnly = true;
        exts = SearchFileTypes::extensionsFor(typeId, typeName);
    }

    d->proxy->applyFilters(terms, llsize, sizeMode, dirsOnly, filesOnly, exts);
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
    emit coreClientDisconnected(_q(c->getHubUrl()));
}

void SearchFrame::slotStopSearch(){
    Q_D(SearchFrame);

    d->stop = true;
    d->waitingResults = false;
    ShareIndex::getInstance()->cancelSearch();
    ClientManager::getInstance()->cancelSearch((void*)this);
}
