/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SearchModel.h"
#include "SearchModelSort.h"
#include "WulforUtil.h"

#include <QFileInfo>
#include <QDir>

bool SearchModel::addResultPtr(const QVariantMap &map){
    try {
        const bool ok = addResult(map["FILE"].toString(),
                  map["SIZE"].toULongLong(),
                  map["TTH"].toString(),
                  map["PATH"].toString(),
                  map["NICK"].toString(),
                  map["FSLS"].toULongLong(),
                  map["ASLS"].toULongLong(),
                  map["IP"].toString(),
                  map["HUB"].toString(),
                  map["HOST"].toString(),
                  map["CID"].toString(),
                  map["ISDIR"].toBool());
        flushDeferredSort();
        return ok;
    }
    catch (SearchListException &) {
        return false;
    }
}

bool SearchModel::addResult
        (
        const QString &file,
        qulonglong size,
        const QString &tth,
        const QString &path,
        const QString &nick,
        const int free_slots,
        const int all_slots,
        const QString &ip,
        const QString &hub,
        const QString &host,
        const QString &cid,
        const bool isDir
        )
{
    if (file.isEmpty())
        return false;

    SearchItem *item;

    QFileInfo file_info(QDir::toNativeSeparators(file));
    QString ext = "";

    if (size > 0)
        ext = file_info.suffix().toUpper();

    SearchItem *parent = rootItem;
    const QString dirKey = dirGroupKey(path, file);

    if (!isDir && tths.contains(tth)) {
        parent = tths[tth];
        if (parent->exists(cid))
            return false;
    } else if (isDir && dirs.contains(dirKey)) {
        parent = dirs[dirKey];
        if (parent->exists(cid))
            return false;
    }

    QList<QVariant> item_data;

    item_data << QVariant() << file << ext << WulforUtil::formatBytes(size)
              << size << tth << path << nick << free_slots
              << all_slots << ip << hub << host;

    item = new SearchItem(item_data, parent);

    if (!item)
        throw SearchListException();

    item->isDir = isDir;
    item->cid = cid;

    if (parent == rootItem) {
        if (!isDir)
            tths.insert(tth, item);
        else
            dirs.insert(dirKey, item);

        auto it = insertSortedSearchItem(sortColumn, sortOrder, parent->childItems, item);
        const int row = static_cast<int>(it - parent->childItems.begin());
        beginInsertRows(QModelIndex(), row, row);
        parent->childItems.insert(it, item);
        endInsertRows();

        return true;
    }

    const QModelIndex parentIdx = createIndexForItem(parent);
    beginInsertRows(parentIdx, parent->childCount(), parent->childCount());
    parent->appendChild(item);
    endInsertRows();

    // Count column display depends on child count; notify without a full resort.
    if (parentIdx.isValid())
        emit dataChanged(parentIdx, parentIdx);

    // Defer Count-column root resort to end of batch (avoids layoutChanged per child).
    if (sortColumn == COLUMN_SF_COUNT)
        countSortPending = true;

    return true;
}
