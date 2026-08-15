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

#include "SearchFrame.h"
#include "SearchFramePrivate.h"
#include "SearchModel.h"
#include "search/SearchListColumns.h"
#include "search/SearchLocalPath.h"
#include "ShareIndex.h"
#include "WulforUtil.h"

#include "dcpp/SearchManager.h"
#include "dcpp/ClientManager.h"
#include "dcpp/SettingsManager.h"

#include <QHeaderView>
#include <QSplitter>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSignalBlocker>

using namespace dcpp;

void SearchFrame::searchAlternates(const QString &tth){
    if (tth.isEmpty())
        return;

    Q_D(SearchFrame);

    d->saveFileType = false;
    lineEdit_SEARCHSTR->setText(tth);
    {
        const QSignalBlocker block(comboBox_FILETYPES);
        comboBox_FILETYPES->setCurrentIndex(SearchManager::TYPE_TTH);
    }
    lineEdit_SIZE->setText("");

    slotStartSearch();
}

void SearchFrame::searchFile(const QString &file){
    if (file.isEmpty())
        return;

    Q_D(SearchFrame);

    d->saveFileType = false;
    lineEdit_SEARCHSTR->setText(file);
    {
        const QSignalBlocker block(comboBox_FILETYPES);
        comboBox_FILETYPES->setCurrentIndex(SearchManager::TYPE_ANY);
    }
    lineEdit_SIZE->setText("");

    slotStartSearch();
}

void SearchFrame::fastSearch(const QString &text, bool isTTH){
    if (text.isEmpty())
        return;

    Q_D(SearchFrame);
    d->saveFileType = false;

    {
        const QSignalBlocker block(comboBox_FILETYPES);
        comboBox_FILETYPES->setCurrentIndex(isTTH ? SearchManager::TYPE_TTH
                                                  : SearchManager::TYPE_ANY);
    }

    lineEdit_SEARCHSTR->setText(text);

    slotStartSearch();
}

void SearchFrame::slotClear(){
    Q_D(SearchFrame);

    d->stop = true;
    d->waitingResults = false;
    d->currentSearch.clear();
    ShareIndex::getInstance()->cancelSearch();
    ClientManager::getInstance()->cancelSearch((void*)this);

    resetResultState();
    lineEdit_SEARCHSTR->clear();
    lineEdit_SIZE->setText("");
}

void SearchFrame::resetResultState(){
    Q_D(SearchFrame);

    if (d->resultFlush)
        d->resultFlush->stop();
    d->pendingResults.clear();
    if (d->mediaEnrich)
        d->mediaEnrich->clearPending();

    treeView_RESULTS->clearSelection();
    d->model->clearModel();
    applyOptionalColumns();
    d->dropped = d->filtered = d->results = 0;
    clearUnseenResults();
}

void SearchFrame::slotResultDoubleClicked(const QModelIndex &index){
    if (!index.isValid())
        return;

    Q_D(SearchFrame);

    QModelIndex i = d->proxy? d->proxy->mapToSource(index) : index;
    SearchItem *item = reinterpret_cast<SearchItem*>(i.internalPointer());
    if (!item)
        return;

    if (!item->isDir) {
        const QString path = item->localPath();
        if (!path.isEmpty()) {
            SearchLocalPath::openDirectory(path);
            return;
        }
    }

    VarMap params;

    if (getDownloadParams(params, item)){
        download(params);

        if (item->childCount() > 0 && !SETTING(DONT_DL_ALREADY_QUEUED)){//download all child items
            QString fname = params["FNAME"].toString();

            for (const auto &child : item->children()){
                if (getDownloadParams(params, child)){
                    params["FNAME"] = fname;

                    download(params);
                }
            }
        }
    }
}

void SearchFrame::slotHeaderMenu(const QPoint&){
    WulforUtil::headerMenu(treeView_RESULTS, SearchListColumns::menuSkip());
}

void SearchFrame::slotToggleSidePanel(){
    QList<int> panes = splitter->sizes();
    Q_D(SearchFrame);

    if (panes[0] < 15){//left pane can't have width less than 15px
        panes[0] = d->left_pane_old_size;
        panes[1] = panes[1] - d->left_pane_old_size;
    }
    else {
        panes[1] = panes[0] + panes[1];
        d->left_pane_old_size = panes[0];
        panes[0] = 0;
    }

    splitter->setSizes(panes);
}
