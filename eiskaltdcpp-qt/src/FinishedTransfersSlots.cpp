/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "FinishedTransfers.h"
#include "FinishedTransfersProxy.h"
#include "SearchFileTypes.h"
#include "WulforSettings.h"

template <bool isUpload>
FinishedTransfers<isUpload>::~FinishedTransfers(){
    QString key = (comboBox->currentIndex() == 0)? WS_FTRANSFERS_FILES_STATE : WS_FTRANSFERS_USERS_STATE;
    WVSET(key, treeView->header()->saveState());

    FinishedManager::getInstance()->removeListener(this);

    model->clearModel();

#ifdef USE_QT_SQLITE
    // Close then drop the registry entry before QApplication teardown
    // (QtSql post-routines abort in sqlite if connections linger).
    const QString conn = db.connectionName();
    db.close();
    db = QSqlDatabase();
    if (!conn.isEmpty())
        QSqlDatabase::removeDatabase(conn);
#endif

    delete proxy;
    delete model;
}

template <bool isUpload>
void FinishedTransfers<isUpload>::slotTypeChanged(int index) {
    QString from_key = (index == 0)? WS_FTRANSFERS_USERS_STATE : WS_FTRANSFERS_FILES_STATE;
    QString to_key = (index == 0)? WS_FTRANSFERS_FILES_STATE : WS_FTRANSFERS_USERS_STATE;
    QByteArray old_state = treeView->header()->saveState();

    if (sender() == comboBox)
        WVSET(from_key, old_state);

    QByteArray state = WVGET(to_key, QByteArray()).toByteArray();
    WulforUtil::restoreTreeHeader(treeView->header(), state);
    treeView->setSortingEnabled(true);

    model->switchViewType(static_cast<FinishedTransfersModel::ViewType>(index));
    proxy->setFileView(index == FinishedTransfersModel::FileView);
    comboBox_FILETYPES->setEnabled(index == FinishedTransfersModel::FileView);

    if (state.isEmpty())
        treeView->sortByColumn(COLUMN_FINISHED_TIME, Qt::AscendingOrder);
    else {
        const int sortCol = treeView->header()->sortIndicatorSection();
        if (sortCol >= 0)
            treeView->sortByColumn(sortCol, treeView->header()->sortIndicatorOrder());
    }
}

template <bool isUpload>
void FinishedTransfers<isUpload>::slotClear() {
    model->clearModel();

    try {
        FinishedManager::getInstance()->removeAll(isUpload);
    }
    catch (const std::exception&){}

#ifdef USE_QT_SQLITE
    if (!db_opened)
        return;

    QSqlQuery q(db);
    q.exec("DROP TABLE files;");
    q.exec("DROP TABLE users;");
#endif
}

template <bool isUpload>
void FinishedTransfers<isUpload>::slotHeaderMenu() {
    WulforUtil::headerMenu(treeView);
}

template <bool isUpload>
void FinishedTransfers<isUpload>::slotSwitchOnlyFull(bool checked) {
    proxy->setFullOnly(checked);
}

template <bool isUpload>
void FinishedTransfers<isUpload>::slotFilterText(const QString &text) {
    proxy->setTextFilter(text);
}

template <bool isUpload>
void FinishedTransfers<isUpload>::slotFileTypeChanged(int index) {
    const QString name = (index >= 0) ? comboBox_FILETYPES->itemText(index) : QString();
    proxy->setExtFilter(SearchFileTypes::extensionsFor(index, name));
}

template <bool isUpload>
void FinishedTransfers<isUpload>::slotSettingsChanged(const QString &key, const QString &) {
    if (key == WS_TRANSLATION_FILE) {
        retranslateUi(this);
        lineEdit_FILTER->setPlaceholderText(tr("Filter"));
        lineEdit_FILTER->setToolTip(tr("Filter by name, path, or user"));
        comboBox_FILETYPES->setToolTip(tr("Filter by file type"));
        const int cur = comboBox_FILETYPES->currentIndex();
        SearchFileTypes::fillCombo(comboBox_FILETYPES);
        if (cur >= 0 && cur < comboBox_FILETYPES->count())
            comboBox_FILETYPES->setCurrentIndex(cur);
    }
}

template FinishedTransfers<true>::~FinishedTransfers();
template FinishedTransfers<false>::~FinishedTransfers();
template void FinishedTransfers<true>::slotTypeChanged(int);
template void FinishedTransfers<false>::slotTypeChanged(int);
template void FinishedTransfers<true>::slotClear();
template void FinishedTransfers<false>::slotClear();
template void FinishedTransfers<true>::slotHeaderMenu();
template void FinishedTransfers<false>::slotHeaderMenu();
template void FinishedTransfers<true>::slotSwitchOnlyFull(bool);
template void FinishedTransfers<false>::slotSwitchOnlyFull(bool);
template void FinishedTransfers<true>::slotFilterText(const QString&);
template void FinishedTransfers<false>::slotFilterText(const QString&);
template void FinishedTransfers<true>::slotFileTypeChanged(int);
template void FinishedTransfers<false>::slotFileTypeChanged(int);
template void FinishedTransfers<true>::slotSettingsChanged(const QString&, const QString&);
template void FinishedTransfers<false>::slotSettingsChanged(const QString&, const QString&);
