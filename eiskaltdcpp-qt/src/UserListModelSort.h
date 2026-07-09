/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include "UserListModel.h"

void userListSort(QList<UserListItem*> &items, int column, Qt::SortOrder order);
QList<UserListItem*>::iterator userListInsertSorted(QList<UserListItem*> &items, UserListItem *item,
                                                    int column, Qt::SortOrder order);
