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
 * File transfer session for Transfer View: baseline + moved bytes, mean rate,
 * progress %, and ETA. Upload scope is peer+file; download scope is our file.
 */
class TransferSession
{
public:
    explicit TransferSession(TransferViewItem *scope);

    /** Group parent when `item` is a child leaf; otherwise `item`. */
    static TransferViewItem *scopeOf(TransferViewItem *item, TransferViewItem *root);

    bool valid() const { return scope_ != nullptr; }
    TransferViewItem *item() const { return scope_; }

    /** Bytes transferred after baseline (parts / download delta). */
    qlonglong movedBytes() const;
    /** Absolute bytes done: baseline + moved (clamped by caller via writeUi). */
    qlonglong progressBytes() const;

    /** Start the session clock once; `baseline` is already-done file bytes. */
    void begin(qlonglong baseline);
    /** Fill SPEED, TLEFT, DPOS, PERC from the session mean. */
    void writeUi(QVariantMap &params, qlonglong fileSize) const;

    /** Upload: every part finished and completed bytes cover the file. */
    bool uploadDone() const;

private:
    TransferViewItem *scope_;

    qlonglong uploadMoved() const;
    qlonglong downloadTotal() const;
};
