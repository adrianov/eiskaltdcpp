/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "PublicHubs.h"
#include "MainWindow.h"

#include <QApplication>
#include <QClipboard>
#include <QItemSelectionModel>
#include <QMenu>

using namespace dcpp;

void PublicHubs::slotContextMenu(){
    QItemSelectionModel *sel_model = treeView->selectionModel();
    QModelIndexList indexes = sel_model->selectedRows(0);

    if (indexes.isEmpty())
        return;

    if (proxy)
        std::transform(indexes.begin(), indexes.end(), indexes.begin(), [&](QModelIndex i) { return proxy->mapToSource(i); });

    WulforUtil *WU = WulforUtil::getInstance();

    QMenu *m = new QMenu();
    QAction *connect = new QAction(WU->getPixmap(WulforUtil::eiCONNECT), tr("Connect"), m);
    QAction *add_fav = new QAction(WU->getPixmap(WulforUtil::eiBOOKMARK_ADD), tr("Add to favorites"), m);
    QAction *copy    = new QAction(WU->getPixmap(WulforUtil::eiEDITCOPY), tr("Copy &address to clipboard"), m);

    m->addActions(QList<QAction*>() << connect << add_fav << copy);

    QAction *ret = m->exec(QCursor::pos());

    m->deleteLater();

    if (ret == connect){
        PublicHubItem * item = nullptr;
        MainWindow *MW = MainWindow::getInstance();

        for (const auto &i : indexes){
            item = reinterpret_cast<PublicHubItem*>(i.internalPointer());

            if (item)
                MW->newHubFrame(item->data(COLUMN_PHUB_ADDRESS).toString(), "");

            item = nullptr;
        }
    }
    else if (ret == add_fav){
        PublicHubItem * item = nullptr;

        for (const auto &i : indexes){
            item = reinterpret_cast<PublicHubItem*>(i.internalPointer());

            if (item && item->entry){
                try{
                    FavoriteManager::getInstance()->addFavorite(*item->entry);
                }
                catch (const std::exception&){}
            }

            item = nullptr;
        }
    }
    else if (ret == copy){
        PublicHubItem * item = nullptr;
        QString out = "";

        for (const auto &i : indexes){
            item = reinterpret_cast<PublicHubItem*>(i.internalPointer());

            if (item)
                out += item->data(COLUMN_PHUB_ADDRESS).toString() + "\n";

            item = nullptr;
        }

        if (!out.isEmpty())
            qApp->clipboard()->setText(out, QClipboard::Clipboard);
    }
}

void PublicHubs::slotHeaderMenu(){
    WulforUtil::headerMenu(treeView);
}

void PublicHubs::slotDoubleClicked(const QModelIndex &index){
    if (!index.isValid())
        return;

    QModelIndex i = proxy? proxy->mapToSource(index) : index;

    PublicHubItem * item = reinterpret_cast<PublicHubItem*>(i.internalPointer());
    MainWindow *MW = MainWindow::getInstance();

    if (item)
        MW->newHubFrame(item->data(COLUMN_PHUB_ADDRESS).toString(), "");
}

void PublicHubs::slotFilter(){
    if (frame->isVisible()){
        treeView->setModel(model);

        disconnect(lineEdit_FILTER, SIGNAL(textChanged(QString)), proxy, SLOT(setFilterFixedString(QString)));

        delete proxy;
        proxy = nullptr;
    }
    else {
        proxy = new PublicHubProxyModel();
        proxy->setDynamicSortFilter(true);
        proxy->setFilterFixedString(lineEdit_FILTER->text());
        proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
        proxy->setFilterKeyColumn(comboBox_FILTER->currentIndex());
        proxy->setSourceModel(model);

        treeView->setModel(proxy);

        connect(lineEdit_FILTER, SIGNAL(textChanged(QString)), proxy, SLOT(setFilterFixedString(QString)));
        connect(comboBox_FILTER, SIGNAL(currentIndexChanged(int)), this, SLOT(slotFilterColumnChanged()));

        lineEdit_FILTER->setFocus();

        if (!lineEdit_FILTER->text().isEmpty())
            lineEdit_FILTER->selectAll();
    }

    frame->setVisible(!frame->isVisible());
}

void PublicHubs::slotFilterColumnChanged(){
    if (proxy)
        proxy->setFilterKeyColumn(comboBox_FILTER->currentIndex());

    if (comboBox_FILTER->hasFocus())
        lineEdit_FILTER->setFocus();
}
