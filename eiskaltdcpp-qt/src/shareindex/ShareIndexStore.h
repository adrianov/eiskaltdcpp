/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#ifdef USE_QT_SQLITE

#include "shareindex/ShareIndexDb.h"

#include <QAtomicInt>
#include <QHash>
#include <QMutex>
#include <QString>

#include <memory>

/** On-disk DuckDB file, engine handle, and per-thread connections. */
class ShareIndexStore
{
public:
    bool isOpen() const { return opened.loadAcquire() != 0; }
    void setOpen(bool on) { opened.storeRelease(on ? 1 : 0); }

    duckdb::Connection *threadConn();
    void disconnectThread();
    void releaseAll();
    /** Open duck at dbFile; false on constructor failure (error in *err). */
    bool openDuck(QString *err);
    void wipeFiles();

    QString dbFile;
    std::unique_ptr<duckdb::DuckDB> duck;
    mutable QMutex openMutex;

private:
    QHash<quintptr, std::shared_ptr<duckdb::Connection>> threadConns;
    QAtomicInt opened{0};
    mutable QMutex connMutex;
};

#endif
