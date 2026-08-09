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
class SearchModel;

/**
 * Column policy for Search results: TTH stays in the model for grouping /
 * magnets / media, but is never a UI column; optional media columns follow
 * whether any result has that field.
 */
class SearchListColumns {
public:
    explicit SearchListColumns(QHeaderView *header);

    /** Hide TTH; show/hide media columns from the current model. */
    void apply(const SearchModel &model);
    /** Columns omitted from the header toggle menu. */
    static QList<int> menuSkip();

private:
    QHeaderView *header_ = nullptr;
};
