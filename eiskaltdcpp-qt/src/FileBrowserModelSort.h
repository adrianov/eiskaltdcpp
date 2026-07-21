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

#include <QList>

void sortFileBrowserItemList(int column, Qt::SortOrder order, QList<FileBrowserItem*> &items);
void sortFileBrowserItems(int column, Qt::SortOrder order, FileBrowserItem *root);
