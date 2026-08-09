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

#include <QList>

class QHeaderView;
class ShareFolderList;

/**
 * Column policy for Share Browser panes: TTH stays in the model for magnets /
 * duplicates / media, but is never a UI column; optional media/stats columns
 * follow listing content; Path/Resolution keep a stable visual order.
 */
class ShareListColumns {
public:
    explicit ShareListColumns(QHeaderView *header);

    /** Hide model-only columns (TTH). Safe after restoreTreeHeader. */
    void hideInternal();
    /** Show/hide media and stats columns from the current folder list. */
    bool syncOptional(const ShareFolderList &list);
    void applyOrder();
    /** hideInternal + syncOptional + applyOrder; true if visibility changed. */
    bool apply(const ShareFolderList &list);

    void setPathVisible(bool on);

    /** Left-pane tree: Name (+ Size); everything else stays hidden. */
    static void hideTreeExtras(QHeaderView *header);
    /** Columns omitted from the right-pane header toggle menu. */
    static QList<int> menuSkip();

private:
    bool setHidden(int col, bool hidden);
    void placeAfter(int afterCol, int moveCol);

    QHeaderView *header_ = nullptr;
};
