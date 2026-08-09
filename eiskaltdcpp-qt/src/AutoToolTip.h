/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

/***
* Origin: http://www.mimec.org/node/337
*/

#pragma once

#include <QStyledItemDelegate>
#include <QHelpEvent>
#include <QAbstractItemView>
#include <QSet>

class AutoToolTipDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    AutoToolTipDelegate(QObject* parent);
    ~AutoToolTipDelegate();

    /** Columns that elide the start of the text (e.g. Path) when the cell is too narrow. */
    void setElideLeftColumns(const QSet<int> &columns);

public slots:
    bool helpEvent(QHelpEvent* e, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index) override;

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;

private:
    QSet<int> elideLeftColumns;
};
