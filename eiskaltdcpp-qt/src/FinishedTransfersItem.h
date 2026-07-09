/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QList>
#include <QVariant>

class FinishedTransfersItem
{
public:
    FinishedTransfersItem(const QList<QVariant> &data, FinishedTransfersItem *parent = nullptr);
    ~FinishedTransfersItem();

    void appendChild(FinishedTransfersItem *child);

    FinishedTransfersItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    FinishedTransfersItem *parent() const;
    void updateColumn(int column, QVariant var);

    QList<FinishedTransfersItem*> childItems;

private:
    QList<QVariant> itemData;
    FinishedTransfersItem *parentItem;
};
