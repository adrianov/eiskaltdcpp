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
#include "MainWindow.h"
#include "WulforUtil.h"
#include "WulforSettings.h"
#include "GlobalTimer.h"

#include "dcpp/StringTokenizer.h"
#include "dcpp/SettingsManager.h"
#include "dcpp/SearchManager.h"
#include "dcpp/ClientManager.h"
#include "dcpp/Util.h"

#include <QMenu>
#include <QPushButton>

using namespace dcpp;

void SearchFrame::slotStartSearch(){
    if (qobject_cast<QPushButton*>(sender()) != pushButton_SEARCH){
        pushButton_SEARCH->click(); //Generating clicked() signal that shows pushButton_STOP button.
                                    //Anybody can suggest something better?
        return;
    }

    Q_D(SearchFrame);

    // Drop prior hub queries from this tab before a new one.
    ClientManager::getInstance()->cancelSearch((void*)this);

    d->stop = false;

    if (lineEdit_SEARCHSTR->text().trimmed().isEmpty())
        return;

    MainWindow *MW = MainWindow::getInstance();
    QString s = lineEdit_SEARCHSTR->text().trimmed();
    StringList clients;

    for (int i = 0; i < d->str_model->rowCount(); i++){
        QModelIndex index = d->str_model->index(i, 0);

        if (index.data(Qt::CheckStateRole).toInt() == Qt::Checked)
            clients.push_back(_tq(index.data().toString()));
    }

#ifndef WITH_DHT
    if (clients.empty())
        return;
#endif

    quint64 llsize = 0;
    int sizeModeInt = SearchManager::SIZE_DONTCARE;
    readSizeFilter(llsize, sizeModeInt);
    const SearchManager::SizeModes searchMode = static_cast<SearchManager::SizeModes>(sizeModeInt);

    rememberSearch(s);

    {
        d->currentSearch = StringTokenizer<string>(s.toStdString(), ' ').getTokens();
        s = "";

        for (auto si = d->currentSearch.begin(); si != d->currentSearch.end(); ) {
            if (si->empty()) {
                si = d->currentSearch.erase(si);
                continue;
            }

            if ((*si)[0] != '-')
                s += QString::fromStdString(*si) + ' ';

            ++si;
        }

        d->token = _q(Util::toString(Util::rand()));
    }

    int ftype = comboBox_FILETYPES->currentIndex();

    d->isHash = (ftype == SearchManager::TYPE_TTH);
    d->filterShared = static_cast<AlreadySharedAction>(comboBox_SHARED->currentIndex());
    d->withFreeSlots = checkBox_FILTERSLOTS->isChecked();

    d->model->setFilterRole(static_cast<int>(d->filterShared));
    if (d->resultFlush)
        d->resultFlush->stop();
    d->pendingResults.clear();
    treeView_RESULTS->clearSelection();
    d->model->clearModel();

    d->dropped = d->filtered = d->results = 0;

    d->viewFilterPending = false;
    applyViewFiltersNow();

    string ftypeStr;
    if (ftype > SearchManager::TYPE_ANY && ftype < SearchManager::TYPE_LAST)
        ftypeStr = SearchManager::getInstance()->getTypeStr(ftype);
    else
    {
        ftypeStr = _tq(lineEdit_SEARCHSTR->text());
        ftype = SearchManager::TYPE_ANY;
    }

    StringList exts;
    try{
        if (ftype == SearchManager::TYPE_ANY){
            // Custom searchtype
            exts = SettingsManager::getInstance()->getExtensions(ftypeStr);
        }
        else if ((ftype > SearchManager::TYPE_ANY && ftype < SearchManager::TYPE_DIRECTORY) || ftype == SearchManager::TYPE_CD_IMAGE){
            // Predefined searchtype
            exts = SettingsManager::getInstance()->getExtensions(string(1, '0' + ftype));
        }
    }
    catch (const SearchTypeException&){
        ftype = SearchManager::TYPE_ANY;
    }

    d->target = s;
    d->searchStartTime = GlobalTimer::getInstance()->getTicks()*1000;

    {
        QStringList terms;
        for (const auto &t : d->currentSearch) {
            if (!t.empty() && t[0] != '-')
                terms << _q(t);
        }
        QStringList localExts;
        for (const auto &e : exts) {
            QString s = _q(e).trimmed().toUpper();
            if (s.startsWith('.'))
                s = s.mid(1);
            if (!s.isEmpty())
                localExts << s;
        }
        const bool dirsOnly = (ftype == SearchManager::TYPE_DIRECTORY);
        const bool filesOnly = (ftype != SearchManager::TYPE_ANY && ftype != SearchManager::TYPE_DIRECTORY);
        SearchFrameLocal::startLocalSearch(this, terms, d->isHash, dirsOnly, filesOnly,
                                          qint64(llsize), int(searchMode), localExts);
    }

    uint64_t maxDelayBeforeSearch = SearchManager::getInstance()->search(clients, s.toStdString(), llsize, SearchManager::TypeModes(ftype), searchMode, d->token.toStdString(), exts, (void*)this);
    uint64_t waitingResultsTime = 20000; // just assumption that user receives most of results in 20 seconds

    d->searchEndTime = d->searchStartTime + maxDelayBeforeSearch + waitingResultsTime;
    d->waitingResults = true;

    if (!checkBox_HIDEPANEL->isChecked()){
        QList<int> panes = splitter->sizes();

        panes[1] = panes[0] + panes[1];

        d->left_pane_old_size = panes[0] > 15 ? panes[0] : d->left_pane_old_size;

        panes[0] = 0;

        splitter->setSizes(panes);
    }

    d->arena_title = tr("Search - %1").arg(s);

    MW->redrawToolPanel();
}

