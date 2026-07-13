/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include "FileBrowserModel.h"

#include <QSortFilterProxyModel>

void sortFileBrowserItems(int column, Qt::SortOrder order, FileBrowserItem *root);

/** Filter proxy that keeps FileBrowserModel::sort (natural Name order). */
class FileBrowserFilterProxy : public QSortFilterProxyModel {
public:
    void sort(int column, Qt::SortOrder order) override {
        if (sourceModel())
            sourceModel()->sort(column, order);
    }
};
