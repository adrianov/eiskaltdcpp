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

#include <QDirModel>
#include <QSet>

/** Folder tree for Preferences → Sharing: checkboxes mark shared roots. */
class ShareDirModel : public QDirModel {
    Q_OBJECT
public:
    explicit ShareDirModel(QObject *parent = nullptr);
    ~ShareDirModel() override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    void setAlias(const QModelIndex &index, const QString &alias);
    void beginExpanding();
    QString filePath(const QModelIndex &index) const;

Q_SIGNALS:
    void getName(QModelIndex);
    void expandMe(QModelIndex);

private:
    QSet<QString> checked;
};
