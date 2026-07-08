/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QAbstractItemModel>
#include <QHash>
#include <QList>
#include <QString>

class ShortcutItem
{
public:
    ShortcutItem(ShortcutItem *parent = nullptr);
    ~ShortcutItem();

    void appendChild(ShortcutItem *child);

    ShortcutItem *child(int row);
    int childCount() const;
    int columnCount() const;
    int row() const;
    ShortcutItem *parent();

    QString title;
    QString shortcut;

private:
    ShortcutItem *parentItem;
    QList<ShortcutItem*> childItems;
};

class ShortcutsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit ShortcutsModel(QObject *parent = nullptr);
    ~ShortcutsModel() override;

    int rowCount(const QModelIndex &index = QModelIndex()) const override;
    int columnCount(const QModelIndex &index = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void repaint() { emit layoutChanged(); }
    void save();

private:
    ShortcutItem *rootItem;
    QHash<ShortcutItem*, QString> items;
};
