/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QMultiHash>
#include <QString>

class TransferViewItem;

namespace TransferViewRemove {

void eraseHashEntry(QMultiHash<QString, TransferViewItem*> &hash, TransferViewItem *item);
/** Offline user with no hub left — skip/create and drop on reconnect keep. */
bool offlineOrphan(const QString &cid, const QString &host = QString());
/** Keep download progress across a short reconnect for the same file only. */
bool keepAcrossReconnect(const TransferViewItem *item, const QString &dl, const QString &ul);
bool matchesRemove(const TransferViewItem *item, bool download, const QString &hub);

} // namespace TransferViewRemove
