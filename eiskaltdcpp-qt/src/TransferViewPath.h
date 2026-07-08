/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QString>

class TransferViewItem;

namespace TransferViewPath {

bool isOpenableTransfer(const QString &filename);
QString resolveTransferPath(const TransferViewItem *item);

} // namespace TransferViewPath
