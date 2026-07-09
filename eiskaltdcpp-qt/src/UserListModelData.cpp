/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "UserListModel.h"
#include "WulforSettings.h"
#include "WulforUtil.h"

#include <QApplication>
#include <QFont>
#include <QPalette>

QVariant UserListModel::data(const QModelIndex & index, int role) const {
    if (!index.isValid())
        return QVariant();

    UserListItem * item = static_cast<UserListItem*>(index.internalPointer());

    if (!item)
        return QVariant();

    switch (role){
        case Qt::DisplayRole:
        {
            switch (index.column()) {
                case COLUMN_NICK: return item->getNick();
                case COLUMN_COMMENT: return item->getComment();
                case COLUMN_TAG: return item->getTag();
                case COLUMN_CONN: return item->getConnection();
                case COLUMN_EMAIL: return item->getEmail();
                case COLUMN_SHARE: return WulforUtil::formatBytes(item->getShare());
                case COLUMN_EXACT_SHARE: return item->getShare();
                case COLUMN_IP: return item->getIP();
            }

            break;
        }
        case Qt::DecorationRole:
        {
            if (index.column() != COLUMN_NICK)
                break;

            return (*WU->getUserIcon(item->getUser(), item->isAway(), item->isOP(), item->getConnection()));

            break;
        }
        case Qt::ToolTipRole:
        {
            if (index.column() == COLUMN_SHARE)
                return QString::number(item->getShare());
            else {

                QString nick = item->getNick();
                WulforUtil::getInstance()->textToHtml(nick, true);

                QString ttip  = "<b>" + headerData(COLUMN_NICK, Qt::Horizontal, Qt::DisplayRole).toString() + "</b>: " + nick + "<br/>";

                QString comment = item->getComment();
                WulforUtil::getInstance()->textToHtml(comment, true);

                ttip += "<b>" + headerData(COLUMN_COMMENT, Qt::Horizontal, Qt::DisplayRole).toString() + "</b>: " + comment + "<br/>";

                QString mail = item->getEmail();
                WulforUtil::getInstance()->textToHtml(mail, true);

                ttip += "<b>" + headerData(COLUMN_EMAIL, Qt::Horizontal, Qt::DisplayRole).toString() + "</b>: " + mail + "<br/>";

                ttip += "<b>" + headerData(COLUMN_IP, Qt::Horizontal, Qt::DisplayRole).toString() + "</b>: " + item->getIP() + "<br/>";
                ttip += "<b>" + headerData(COLUMN_SHARE, Qt::Horizontal, Qt::DisplayRole).toString() + "</b>: " +
                        WulforUtil::formatBytes(item->getShare()) + "<br/>";

                QString tag = item->getTag();
                WulforUtil::getInstance()->textToHtml(tag, true);

                ttip += "<b>" + headerData(COLUMN_TAG, Qt::Horizontal, Qt::DisplayRole).toString() + "</b>: " + tag + "<br/>";

                QString connection = item->getConnection();
                WulforUtil::getInstance()->textToHtml(connection, true);

                ttip += "<b>" + headerData(COLUMN_CONN, Qt::Horizontal, Qt::DisplayRole).toString() + "</b>: " + connection + "<br/>";

                if (item->isOP())
                    ttip += tr("<b>Hub role</b>: Operator");
                else
                    ttip += tr("<b>Hub role</b>: User");

                if (item->isFav())
                    ttip += tr("<br/><b>Favorite user</b>");

                return ttip;
            }

            break;
        }
        case Qt::TextAlignmentRole:
        {
            if (index.column() == COLUMN_SHARE)
                return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);

            break;
        }
        case Qt::FontRole:
        {
            // Default QFont() can paint invisible on Fusion/macOS; bold the view font instead.
            if (item->isFav() && WBGET(WB_CHAT_HIGHLIGHT_FAVS)) {
                QFont font = qApp->font();
                font.setBold(true);
                return font;
            }

            break;
        }
        case Qt::ForegroundRole:
        {
            if (item->isFav() && WBGET(WB_CHAT_HIGHLIGHT_FAVS))
                return qApp->palette().color(QPalette::Text);

            break;
        }
    }

    return QVariant();
}


QVariant UserListModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if ((orientation == Qt::Horizontal) && (role == Qt::DisplayRole)) {
        switch (section) {
            case COLUMN_NICK: return tr("Nick");
            case COLUMN_COMMENT: return tr("Comment");
            case COLUMN_TAG: return tr("Tag");
            case COLUMN_CONN: return tr("Connection");
            case COLUMN_EMAIL: return tr("E-mail");
            case COLUMN_SHARE: return tr("Share");
            case COLUMN_EXACT_SHARE: return tr("Exact share size");
            case COLUMN_IP: return tr("IP");
        }
    }

    return QVariant();
}



