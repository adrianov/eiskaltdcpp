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

#include <QString>
#include <QStringList>

/** Value types for ShareIndex search, holders, media, and HUD. */
struct ShareIndexModels {
    enum Source {
        SourceFileList = 1,
        SourceHubSearch = 2
    };

    struct SearchFilter {
        QStringList terms;
        QStringList extensions; // uppercase, no dot; empty = any
        bool isHash = false;
        bool dirsOnly = false;
        bool filesOnly = false;
        qint64 size = 0;
        int sizeMode = 0;
        int limit = 500;
    };

    /** Unique index holders per TTH (nick + cid + size). */
    struct IndexUser {
        QString nick;
        QString cid;
        qint64 size = 0;
    };

    /** Media from indexed file lists (empty fields omitted from hashes). */
    struct MediaInfo {
        int bitrate = 0;
        QString resolution;
        QString video;
        QString audio;
        bool isEmpty() const
        {
            return bitrate <= 0 && resolution.isEmpty() && video.isEmpty() && audio.isEmpty();
        }
    };

    /** Fast index HUD: entry_count meta + on-disk DB size. */
    struct IndexStats {
        qint64 files = 0; // share_index_meta.entry_count (files + dirs)
        qint64 dbBytes = 0;
    };
};
