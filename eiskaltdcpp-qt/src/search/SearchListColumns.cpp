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

#include "search/SearchListColumns.h"
#include "SearchModel.h"

#include <QHeaderView>

SearchListColumns::SearchListColumns(QHeaderView *header)
    : header_(header)
{
}

void SearchListColumns::apply(const SearchModel &model)
{
    if (!header_)
        return;
    for (int col : menuSkip())
        header_->hideSection(col);
    header_->setSectionHidden(COLUMN_SF_BR, !model.hasBitrate());
    header_->setSectionHidden(COLUMN_SF_WH, !model.hasResolution());
    header_->setSectionHidden(COLUMN_SF_MVIDEO, !model.hasVideo());
    header_->setSectionHidden(COLUMN_SF_MAUDIO, !model.hasAudio());
}

QList<int> SearchListColumns::menuSkip()
{
    return { COLUMN_SF_TTH };
}
