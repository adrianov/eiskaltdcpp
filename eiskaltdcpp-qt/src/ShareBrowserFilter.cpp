/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "ShareBrowser.h"
#include "SearchFileTypes.h"
#include "FileBrowserFilterProxy.h"

#include "dcpp/SearchManager.h"
#include "dcpp/Util.h"
#include "dcpp/Text.h"

#include <QSignalBlocker>

using namespace dcpp;

void ShareBrowser::readSizeFilter(quint64 &size, int &mode) const {
    const QString str_size = lineEdit_SIZE->text();
    double lsize = Util::toDouble(Text::fromT(str_size.toStdString()));

    switch (comboBox_SIZE->currentIndex()) {
        case 1: lsize *= 1024.0; break;
        case 2: lsize *= (1024.0 * 1024.0); break;
        case 3: lsize *= (1024.0 * 1024.0 * 1024.0); break;
    }

    size = static_cast<quint64>(lsize);
    if (!size || str_size.isEmpty()) {
        mode = SearchManager::SIZE_DONTCARE;
        return;
    }

    switch (comboBox_SIZETYPE->currentIndex()) {
        case 0: mode = SearchManager::SIZE_ATLEAST; break;
        default: mode = SearchManager::SIZE_ATMOST; break;
    }
}

void ShareBrowser::slotClearFilters() {
    const QSignalBlocker blockFilter(lineEdit_FILTER);
    const QSignalBlocker blockSize(lineEdit_SIZE);
    const QSignalBlocker blockTypes(comboBox_FILETYPES);
    const QSignalBlocker blockSizeType(comboBox_SIZETYPE);
    const QSignalBlocker blockUnit(comboBox_SIZE);

    lineEdit_FILTER->clear();
    lineEdit_SIZE->clear();
    comboBox_FILETYPES->setCurrentIndex(0);
    comboBox_SIZETYPE->setCurrentIndex(0);
    comboBox_SIZE->setCurrentIndex(2);

    viewFilterPending = false;
    applyViewFiltersNow();
}

void ShareBrowser::slotApplyFilters() {
    if (viewFilterPending)
        return;
    viewFilterPending = true;
    QMetaObject::invokeMethod(this, "flushViewFilters", Qt::QueuedConnection);
}

void ShareBrowser::flushViewFilters() {
    if (!viewFilterPending)
        return;
    viewFilterPending = false;
    applyViewFiltersNow();
}

void ShareBrowser::applyViewFiltersNow() {
    const QStringList terms =
            lineEdit_FILTER->text().trimmed().split(QLatin1Char(' '), WULFOR_SKIP_EMPTY);

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
    } else if (typeId != SearchManager::TYPE_ANY && typeId != SearchManager::TYPE_TTH) {
        // TTH is hub-search only; treat like Any for the local folder list.
        filesOnly = true;
        exts = SearchFileTypes::extensionsFor(typeId, typeName);
    }

    if (proxy)
        proxy->applyFilters(terms, llsize, sizeMode, dirsOnly, filesOnly, exts, lineEdit_PATH->text());
    if (tree_proxy)
        tree_proxy->applyFilters(terms, llsize, sizeMode, dirsOnly, filesOnly, exts, QString());
}

QModelIndex ShareBrowser::treeMapToSource(const QModelIndex &index) const {
    return tree_proxy ? tree_proxy->mapToSource(index) : index;
}

QModelIndex ShareBrowser::treeMapFromSource(const QModelIndex &index) const {
    return (tree_proxy && index.isValid()) ? tree_proxy->mapFromSource(index) : index;
}
