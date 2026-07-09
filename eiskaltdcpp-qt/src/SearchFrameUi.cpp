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
#include "SearchModel.h"
#include "WulforUtil.h"
#include "GlobalTimer.h"

#include "dcpp/SearchManager.h"

#include <QHeaderView>
#include <QSplitter>
#include <QListWidget>
#include <QListWidgetItem>

using namespace dcpp;

void SearchFrame::searchAlternates(const QString &tth){
    if (tth.isEmpty())
        return;

    Q_D(SearchFrame);

    lineEdit_SEARCHSTR->setText(tth);
    comboBox_FILETYPES->setCurrentIndex(SearchManager::TYPE_TTH);
    lineEdit_SIZE->setText("");

    slotStartSearch();

    d->saveFileType = false;
}

void SearchFrame::searchFile(const QString &file){
    if (file.isEmpty())
        return;

    Q_D(SearchFrame);

    lineEdit_SEARCHSTR->setText(file);
    comboBox_FILETYPES->setCurrentIndex(SearchManager::TYPE_ANY);
    lineEdit_SIZE->setText("");

    d->saveFileType = false;

    slotStartSearch();
}

void SearchFrame::fastSearch(const QString &text, bool isTTH){
    if (text.isEmpty())
        return;

    if (!isTTH)
        comboBox_FILETYPES->setCurrentIndex(0); // set type "Any"
    else
        comboBox_FILETYPES->setCurrentIndex(8); // set type "TTH"

    lineEdit_SEARCHSTR->setText(text);

    slotStartSearch();
}

void SearchFrame::slotClear(){
    Q_D(SearchFrame);

    treeView_RESULTS->clearSelection();
    d->model->clearModel();
    lineEdit_SEARCHSTR->clear();
    lineEdit_SIZE->setText("");

    d->dropped = d->results = 0;
}

void SearchFrame::slotResultDoubleClicked(const QModelIndex &index){
    if (!index.isValid() || !index.internalPointer())
        return;

    Q_D(SearchFrame);

    QModelIndex i = d->proxy? d->proxy->mapToSource(index) : index;

    SearchItem *item = reinterpret_cast<SearchItem*>(i.internalPointer());
    VarMap params;

    if (getDownloadParams(params, item)){
        download(params);

        if (item->childCount() > 0 && !SETTING(DONT_DL_ALREADY_QUEUED)){//download all child items
            QString fname = params["FNAME"].toString();

            for (const auto &i : item->childItems){
                if (getDownloadParams(params, i)){
                    params["FNAME"] = fname;

                    download(params);
                }
            }
        }
    }
}

void SearchFrame::slotHeaderMenu(const QPoint&){
    WulforUtil::headerMenu(treeView_RESULTS);
}

void SearchFrame::slotTimer(){
    Q_D(SearchFrame);

#ifdef USE_QT_SQLITE
    if (++d->indexStatsTick >= 30) {
        d->indexStatsTick = 0;
        SearchFrameLocal::refreshIndexStats(this);
    }
#endif

    if (d->waitingResults) {
        uint64_t now = GlobalTimer::getInstance()->getTicks()*1000;
        float fraction  = 100.0f*(now - d->searchStartTime)/(d->searchEndTime - d->searchStartTime);
        if (fraction >= 100.0) {
            fraction = 100.0;
            d->waitingResults = false;
        }
#if defined(USE_PROGRESS_BARS)
        const QString msg = tr("Searching for %1 ...").arg(d->target);
        progressBar->setFormat(msg);
        progressBar->setValue(static_cast<unsigned>(fraction));
#else
        const QString msg = tr("Search progress of \"%1\" is %2\%")
                .arg(d->target)
                .arg(QString::number(fraction, 'f', 1));
        progressIndicator->setText(msg);
#endif
    }
    else {
#if defined(USE_PROGRESS_BARS)
        progressBar->setFormat(QString());
        progressBar->setValue(0);
#else
        progressIndicator->clear();
#endif
        lineEdit_SEARCHSTR->setEnabled(true);
    }

    if (d->dropped == d->results && !d->dropped){

        if (d->currentSearch.empty())
            frame_PROGRESS->hide();
        else {
            frame_PROGRESS->show();

            QString text = QString(tr("<b>No results</b>"));

            status->setText(text);
        }
    }
    else {
        if (!frame_PROGRESS->isVisible())
            frame_PROGRESS->show();

        QString text = QString(tr("Found: <b>%1</b>  Dropped: <b>%2</b>")).arg(d->results).arg(d->dropped);

        status->setText(text);
    }
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

void SearchFrame::setIndexStats(const QString &text)
{
#ifdef USE_QT_SQLITE
    label_INDEX_STATS->setText(text);
#else
    Q_UNUSED(text);
#endif
}

