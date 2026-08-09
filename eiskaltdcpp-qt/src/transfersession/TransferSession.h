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

#include <QVariantMap>
#include <QtGlobal>

class TransferViewItem;

/**
 * Session counters for Transfer View: moved bytes, mean rate, progress, ETA.
 *
 * Upload: one session per peer+file across segments.
 * Download file group: full file from queue baseline.
 * Download peer: peer byte sum vs file left when that peer's first segment starts
 * (`speedBase` holds left-at-join; see TransferSessionRow).
 */
class TransferSession
{
public:
    explicit TransferSession(TransferViewItem *scope);

    /** Group parent when `item` is a child leaf; otherwise `item`. */
    static TransferViewItem *scopeOf(TransferViewItem *item, TransferViewItem *root);

    bool valid() const { return scope_ != nullptr; }
    TransferViewItem *item() const { return scope_; }

    qlonglong movedBytes() const;
    qlonglong progressBytes() const;

    void begin(qlonglong baseline);
    void writeUi(QVariantMap &params, qlonglong fileSize) const;
    bool uploadDone() const;

private:
    TransferViewItem *scope_;

    qlonglong uploadMoved() const;
    qlonglong downloadTotal() const;
};
