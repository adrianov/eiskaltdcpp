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

#include <QStyledItemDelegate>

/**
 * Floor row height to the list icon size.
 *
 * QTreeView remasures from the leftmost *visible* column. When an icon column
 * scrolls off, text-only cells report a shorter hint and the row collapses.
 * Every column reports at least the icon height so scrolling sideways is stable.
 */
class IconRowDelegate : public QStyledItemDelegate
{
public:
    static constexpr int kNone = 0;
    static constexpr int kFileIcon = 16;
    static constexpr int kTransferIcon = 18;

    explicit IconRowDelegate(QObject *parent = nullptr, int iconSide = kFileIcon);

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    int iconSide_;
};
