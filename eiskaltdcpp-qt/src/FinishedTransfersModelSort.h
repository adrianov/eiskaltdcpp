/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QList>

#include "FinishedTransfersModel.h"

namespace FinishedTransfersModelSort {

void sortFiles(int column, Qt::SortOrder order, QList<FinishedTransfersItem*> &items);
void sortUsers(int column, Qt::SortOrder order, QList<FinishedTransfersItem*> &items);

} // namespace FinishedTransfersModelSort
