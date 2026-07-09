/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "SideBar.h"
#include "WulforUtil.h"
#include "WulforSettings.h"
#include "ArenaWidgetManager.h"
#include "ArenaWidgetFactory.h"
#include "QuickConnect.h"
#include "SearchFrame.h"
#include "ShareBrowser.h"

#include <QMenu>
#include <QItemSelectionModel>
#include <QFileDialog>
#include <QDesktopServices>
#include <QDir>
#include <QUrl>
#include <QHeaderView>

using namespace dcpp;

static const QString &SIDEBAR_SHOW_CLOSEBUTTONS = "mainwindow/sidebar-with-close-buttons";

void SideBarView::slotSidebarContextMenu(){
    QItemSelectionModel *s_m = selectionModel();
    QModelIndexList selected = s_m->selectedRows(0);

    if (selected.size() < 1)
        return;

    SideBarItem *item = reinterpret_cast<SideBarItem*>(selected.at(0).internalPointer());

    QMenu *menu = nullptr;
    
    if (item && item->childCount() > 0){
        menu = new QMenu(this);
        menu->addAction(WICON(WulforUtil::eiEDITDELETE), tr("Close all"));

        if (menu->exec(QCursor::pos())){
            QList<SideBarItem*> children = item->childItems;

            for (const auto &child : children){
                if (child && child->getWidget())
                    ArenaWidgetManager::getInstance()->rem(child->getWidget());
            }
        }
        
        menu->deleteLater();
    }
    else if (item && item->getWidget()){
        menu = item->getWidget()->getMenu();
        
        if (!menu && (item->getWidget()->state() & ArenaWidget::Singleton))
            return;

        if(!menu){
            menu = new QMenu(this);
            menu->addAction(WICON(WulforUtil::eiEDITDELETE), tr("Close"));

            if (menu->exec(QCursor::pos()))
                ArenaWidgetManager::getInstance()->rem(item->getWidget());
            
            menu->deleteLater();
        }
        else
            menu->exec(QCursor::pos());
    }
}

void SideBarView::slotSidebarHook(const QModelIndex &index){
    if (index.column() == 1){
        SideBarItem *item = reinterpret_cast<SideBarItem*>(index.internalPointer());

        if (item->getWidget()){
            switch (item->getWidget()->role()){
            case ArenaWidget::Hub:
            case ArenaWidget::PrivateMessage:
            case ArenaWidget::Search:
            case ArenaWidget::ShareBrowser:
            case ArenaWidget::CustomWidget:
                ArenaWidgetManager::getInstance()->rem(item->getWidget());
                break;
            default:
                break;
            }
        }
    }
}

void SideBarView::slotSideBarDblClicked(const QModelIndex &index){
    if (index.column())
        return;

    SideBarItem *item = reinterpret_cast<SideBarItem*>(index.internalPointer());

    if (!_model->isRootItem(item) || item->childCount() > 0)
        return;

    switch (_model->rootItemRole(item)){
    case ArenaWidget::Search:
    {
        ArenaWidgetFactory().create<SearchFrame>();

        break;
    }
    case ArenaWidget::Hub:
    {
        QuickConnect qc;

        qc.exec();

        break;
    }
        //FIXME: Next code duplicates methods from MainWindow
    case ArenaWidget::ShareBrowser:
    {
        QString file = QFileDialog::getOpenFileName ( this, tr ( "Choose file to open" ),
                                                      QString::fromStdString ( Util::getPath ( Util::PATH_FILE_LISTS ) ),
                                                      tr ( "Modern XML Filelists" ) + " (*.xml.bz2);;" +
                                                      tr ( "Modern XML Filelists uncompressed" ) + " (*.xml);;" +
                                                      tr ( "All files" ) + " (*)" );

        if ( file.isEmpty() )
            return;

        file = QDir::toNativeSeparators ( file );
        UserPtr user = dcpp::DirectoryListing::getUserFromFilename ( _tq ( file ) );

        if ( user )
            ArenaWidgetFactory().create<ShareBrowser, UserPtr, QString, QString> ( user, file, "" );

        break;
    }
    case ArenaWidget::PrivateMessage:
    {
        QString f = QFileDialog::getOpenFileName(this, tr("Open log file"),_q(SETTING(LOG_DIRECTORY)), tr("Log files (*.log);;All files (*.*)"));

        if ( !f.isEmpty() ) {
            f = QDir::toNativeSeparators ( f );

            if ( f.startsWith ( "/" ) )
                f = "file://" + f;
            else
                f = "file:///" + f;

            QDesktopServices::openUrl ( QUrl(f) );
        }

        break;
    }
    default:
        break;
    }

    setExpanded(index, true);
}

void SideBarView::slotWidgetActivated ( QModelIndex i ) {
    selectionModel()->clearSelection();
    selectionModel()->select(i, QItemSelectionModel::SelectCurrent|QItemSelectionModel::Rows);
}

void SideBarView::slotUpdateHeaderSize()
{
    const int width = viewport()->width();
    if (width <= 0)
        return;

    if (WBGET(SIDEBAR_SHOW_CLOSEBUTTONS, true)){
        header()->showSection(1);
        header()->resizeSection(1, 30);
        header()->resizeSection(0, width - 32);
    }
    else{
        header()->hideSection(1);
        header()->resizeSection(0, width);
    }
}

