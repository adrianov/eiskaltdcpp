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

#include "dcpp/SearchManager.h"
#include "dcpp/SettingsManager.h"

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

    d->dropped = d->filtered = d->results = 0;
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
