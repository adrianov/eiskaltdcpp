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

#include "sharebrowser/ShareListColumns.h"
#include "sharebrowser/ShareFolderList.h"
#include "FileBrowserModel.h"

#include <QHeaderView>

ShareListColumns::ShareListColumns(QHeaderView *header)
    : header_(header)
{
}

void ShareListColumns::hideInternal()
{
    if (!header_)
        return;
    header_->hideSection(COLUMN_FILEBROWSER_TTH);
}

bool ShareListColumns::setHidden(int col, bool hidden)
{
    if (!header_ || header_->isSectionHidden(col) == hidden)
        return false;
    header_->setSectionHidden(col, hidden);
    return true;
}

bool ShareListColumns::syncOptional(const ShareFolderList &list)
{
    if (!header_)
        return false;
    bool changed = setHidden(COLUMN_FILEBROWSER_TTH, true);
    changed = setHidden(COLUMN_FILEBROWSER_BR, !list.hasBitrate()) || changed;
    changed = setHidden(COLUMN_FILEBROWSER_WH, !list.hasResolution()) || changed;
    changed = setHidden(COLUMN_FILEBROWSER_MVIDEO, !list.hasVideo()) || changed;
    changed = setHidden(COLUMN_FILEBROWSER_MAUDIO, !list.hasAudio()) || changed;
    changed = setHidden(COLUMN_FILEBROWSER_HIT, !list.hasDownloaded()) || changed;
    changed = setHidden(COLUMN_FILEBROWSER_TS, !list.hasShared()) || changed;
    return changed;
}

void ShareListColumns::placeAfter(int afterCol, int moveCol)
{
    if (!header_ || header_->isSectionHidden(moveCol) || header_->isSectionHidden(afterCol))
        return;
    const int afterVis = header_->visualIndex(afterCol);
    const int moveVis = header_->visualIndex(moveCol);
    if (afterVis < 0 || moveVis < 0)
        return;
    // When moveCol is before afterCol, target is afterVis (removal shifts left).
    const int to = afterVis + (moveVis > afterVis ? 1 : 0);
    if (moveVis != to)
        header_->moveSection(moveVis, to);
}

void ShareListColumns::applyOrder()
{
    placeAfter(COLUMN_FILEBROWSER_NAME, COLUMN_FILEBROWSER_PATH);
    placeAfter(COLUMN_FILEBROWSER_SIZE, COLUMN_FILEBROWSER_WH);
}

bool ShareListColumns::apply(const ShareFolderList &list)
{
    const bool changed = syncOptional(list);
    applyOrder();
    return changed;
}

void ShareListColumns::setPathVisible(bool on)
{
    if (header_)
        header_->setSectionHidden(COLUMN_FILEBROWSER_PATH, !on);
}

void ShareListColumns::hideTreeExtras(QHeaderView *header)
{
    if (!header)
        return;
    header->hideSection(COLUMN_FILEBROWSER_ESIZE);
    header->hideSection(COLUMN_FILEBROWSER_TTH);
    header->hideSection(COLUMN_FILEBROWSER_BR);
    header->hideSection(COLUMN_FILEBROWSER_WH);
    header->hideSection(COLUMN_FILEBROWSER_MVIDEO);
    header->hideSection(COLUMN_FILEBROWSER_MAUDIO);
    header->hideSection(COLUMN_FILEBROWSER_HIT);
    header->hideSection(COLUMN_FILEBROWSER_TS);
    header->hideSection(COLUMN_FILEBROWSER_PATH);
}

QList<int> ShareListColumns::menuSkip()
{
    return {COLUMN_FILEBROWSER_TTH};
}
