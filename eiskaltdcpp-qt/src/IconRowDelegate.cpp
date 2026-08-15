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

#include "IconRowDelegate.h"

#include <QStyle>
#include <QWidget>

IconRowDelegate::IconRowDelegate(QObject *parent, int iconSide)
    : QStyledItemDelegate(parent)
    , iconSide_(iconSide)
{
}

QSize IconRowDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    if (iconSide_ <= 0)
        return size;

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.features |= QStyleOptionViewItem::HasDecoration;
    opt.decorationSize = QSize(iconSide_, iconSide_);
    if (const QWidget *w = opt.widget)
        size.setHeight(qMax(size.height(),
            w->style()->sizeFromContents(QStyle::CT_ItemViewItem, &opt, QSize(), w).height()));
    return size;
}
