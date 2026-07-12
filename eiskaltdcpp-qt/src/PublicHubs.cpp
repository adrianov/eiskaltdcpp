/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "PublicHubs.h"
#include "WulforSettings.h"
#include "AutoToolTip.h"

#include "dcpp/ClientManager.h"

#include <QHeaderView>
#include <QKeyEvent>

using namespace dcpp;

PublicHubs::PublicHubs(QWidget *parent) :
    QWidget(parent), proxy(nullptr)
{
    setupUi(this);

    setUnload(false);

    model = new PublicHubModel();

    treeView->setModel(model);
    treeView->setItemDelegate(new AutoToolTipDelegate(treeView));
    WulforUtil::restoreTreeHeader(treeView->header(), WVGET(WS_PUBLICHUBS_STATE, QByteArray()).toByteArray());
    treeView->sortByColumn(COLUMN_PHUB_USERS, Qt::DescendingOrder);

    lineEdit_FILTER->installEventFilter(this);

    FavoriteManager::getInstance()->addListener(this);
    ClientManager::getInstance()->addListener(this);

    QString hubs = _q(SettingsManager::getInstance()->get(SettingsManager::HUBLIST_SERVERS));

    comboBox_HUBS->addItems(hubs.split(";", WULFOR_SKIP_EMPTY));
    comboBox_HUBS->setCurrentIndex(FavoriteManager::getInstance()->getSelectedHubList());

    for (int i = 0; i < model->columnCount(); i++)
        comboBox_FILTER->addItem(model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString());

    comboBox_FILTER->setCurrentIndex(COLUMN_PHUB_DESC);

    frame->hide();

    entries = FavoriteManager::getInstance()->getPublicHubs();
    fillCountries();
    updateList();

    if(FavoriteManager::getInstance()->isDownloading()) {
        label_STATUS->setText(tr("Downloading public hub list..."));
    } else if(entries.empty()) {
        FavoriteManager::getInstance()->refresh();
    }

    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    treeView->header()->setContextMenuPolicy(Qt::CustomContextMenu);

    toolButton_CLOSEFILTER->setIcon(WICON(WulforUtil::eiEDITDELETE));

    connect(this, SIGNAL(coreDownloadStarted(QString)),  this, SLOT(setStatus(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreDownloadFailed(QString)),   this, SLOT(setStatus(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreDownloadFinished(QString)), this, SLOT(onFinished(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreCacheLoaded(QString)),      this, SLOT(onFinished(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreClientChanged()),           this, SLOT(slotClientChanged()), Qt::QueuedConnection);

    connect(treeView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotContextMenu()));
    connect(treeView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(slotDoubleClicked(QModelIndex)));
    connect(treeView->header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotHeaderMenu()));
    connect(toolButton_CLOSEFILTER, SIGNAL(clicked()), this, SLOT(slotFilter()));
    connect(comboBox_HUBS, SIGNAL(activated(int)), this, SLOT(slotHubChanged(int)));
    connect(comboBox_COUNTRY, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCountryChanged(int)));
    connect(WulforSettings::getInstance(), SIGNAL(strValueChanged(QString,QString)), this, SLOT(slotSettingsChanged(QString,QString)));
    
    ArenaWidget::setState( ArenaWidget::Flags(ArenaWidget::state() | ArenaWidget::Singleton | ArenaWidget::Hidden) );
    if (comboBox_HUBS->count())
        slotHubChanged(comboBox_HUBS->currentIndex());
}

PublicHubs::~PublicHubs(){
    WVSET(WS_PUBLICHUBS_STATE, treeView->header()->saveState());
    
    delete model;
    delete proxy;

    FavoriteManager::getInstance()->removeListener(this);
    ClientManager::getInstance()->removeListener(this);
}

bool PublicHubs::eventFilter(QObject *obj, QEvent *e){
    if (e->type() == QEvent::KeyRelease){
        QKeyEvent *k_e = reinterpret_cast<QKeyEvent*>(e);

        if (static_cast<LineEdit*>(obj) == lineEdit_FILTER && k_e->key() == Qt::Key_Escape){
            lineEdit_FILTER->clear();

            requestFilter();

            return true;
        }
    }

    return QWidget::eventFilter(obj, e);
}

void PublicHubs::closeEvent(QCloseEvent *e){
    isUnload()? e->accept() : e->ignore();
}

void PublicHubs::setStatus(const QString &stat){
    label_STATUS->setText(stat);
}

void PublicHubs::onFinished(const QString &stat){
    setStatus(stat);

    entries = FavoriteManager::getInstance()->getPublicHubs();

    fillCountries();
    updateList();
}

void PublicHubs::slotHubChanged(int pos){
    FavoriteManager::getInstance()->setHubList(pos);
    FavoriteManager::getInstance()->refresh();
}

void PublicHubs::slotSettingsChanged(const QString &key, const QString&){
    if (key == WS_TRANSLATION_FILE) {
        retranslateUi(this);
        fillCountries();
    }
}

void PublicHubs::slotClientChanged(){
    if (model)
        model->refreshConnected();
}

void PublicHubs::on(DownloadStarting, const std::string& l) noexcept{
    emit coreDownloadStarted(tr("Downloading public hub list... (%1)").arg(_q(l)));
}

void PublicHubs::on(DownloadFailed, const std::string& l) noexcept{
    emit coreDownloadFailed(tr("Download failed: %1").arg(_q(l)));
}

void PublicHubs::on(DownloadFinished, const std::string& l) noexcept{
    emit coreDownloadFinished(tr("Hub list downloaded... (%1)").arg(_q(l)));
}

void PublicHubs::on(LoadedFromCache, const std::string& l, const std::string& d) noexcept{
    emit coreCacheLoaded(tr("Locally cached (as of %1) version of the hub list loaded (%2)").arg(_q(d)).arg(_q(l)));
}

void PublicHubs::on(Corrupted, const string& l) noexcept {
    if (l.empty())
        emit coreDownloadFailed(tr("Cached hub list is corrupted or unsupported"));
    else
        emit coreDownloadFailed(tr("Downloaded hub list is corrupted or unsupported (%1)").arg(_q(l)));
}

void PublicHubs::on(ClientConnected, Client*) noexcept{
    emit coreClientChanged();
}

void PublicHubs::on(ClientDisconnected, Client*) noexcept{
    emit coreClientChanged();
}
