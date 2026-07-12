/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QStringListModel>
#include <QList>
#include <QString>

/** Checkable hub list model for the search side panel. */
class SearchStringListModel: public QStringListModel {
public:
    SearchStringListModel(QObject *parent = nullptr): QStringListModel(parent) {}
    virtual ~SearchStringListModel() {}

    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &) const {
        return (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
    }

private:
    QList<QString> checked;
};
