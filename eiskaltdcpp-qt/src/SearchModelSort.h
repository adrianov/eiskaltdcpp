/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include "SearchModel.h"

void sortSearchItems(int column, Qt::SortOrder order, QList<SearchItem*> &items);
QList<SearchItem*>::iterator insertSortedSearchItem(int column, Qt::SortOrder order,
                                                    QList<SearchItem*> &items, SearchItem *item);
