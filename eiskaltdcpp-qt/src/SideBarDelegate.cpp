/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "SideBar.h"
#include "WulforUtil.h"

#include <QPainter>

SideBarDelegate::SideBarDelegate(QObject *parent):
    QStyledItemDelegate(parent)
{
}

SideBarDelegate::~SideBarDelegate(){
}

void SideBarDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const{
    if (!index.column()){
        QStyledItemDelegate::paint(painter, option, index);

        return;
    }

    bool showCloseBtn = false;
    SideBarItem *item = reinterpret_cast<SideBarItem*>(index.internalPointer());
    ArenaWidget *awgt = item->getWidget();

    if (awgt){
        switch (awgt->role()){
        case ArenaWidget::Hub:
        case ArenaWidget::PrivateMessage:
        case ArenaWidget::Search:
        case ArenaWidget::ShareBrowser:
        case ArenaWidget::CustomWidget:
            showCloseBtn = true;
        default:
            break;
        }
    }

    QStyledItemDelegate::paint(painter, option, index);

    if ((option.state & (QStyle::State_MouseOver | QStyle::State_Selected)) && showCloseBtn){
        QPixmap px = WICON_SIZE(WulforUtil::eiEDITDELETE, 16);

        painter->drawPixmap(option.rect.x() + (option.rect.width() - 16)/2,
                            option.rect.y() + (option.rect.height() - 16)/2,
                            px);

        return;
    }
}

QSize SideBarDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const{
    QVariant value = index.data(Qt::SizeHintRole);
    if (value.isValid())
        return qvariant_cast<QSize>(value);

    static const int MARGIN = 1;
    const int PXHEIGHT = option.fontMetrics.height() > 16? option.fontMetrics.height() : 16;
    const int HEIGHT = PXHEIGHT+MARGIN*4;

    return QSize( 200, HEIGHT );
}

