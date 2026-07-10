/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ShareIndex.h"
#include "ShareIndexQueueCore.h"

#ifdef USE_QT_SQLITE

void ShareIndex::reclaimFreePages(duckdb::Connection &con)
{
    if (ShareIndexWriteQueue::isStopping())
        return;
    QString err;
    if (!ShareIndexDb::execOk(con, "CHECKPOINT", &err))
        setLastError(err);
}

#endif
