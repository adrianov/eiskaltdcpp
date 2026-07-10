/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QSortFilterProxyModel>
#include <QString>
#include <QStringList>
#include <string>

/** Filters finished transfers by full-only, file-list hide, text, and file type. */
class FinishedTransferProxyModel: public QSortFilterProxyModel {
    Q_OBJECT

public:
    FinishedTransferProxyModel(bool hideFileLists = false, bool requireFullFile = false) :
        hideFileLists_(hideFileLists), requireFullFile_(requireFullFile),
        fullOnly_(false), fileView_(true) {}

    void sort(int column, Qt::SortOrder order) override {
        if (sourceModel())
            sourceModel()->sort(column, order);
    }

    void setFullOnly(bool fullOnly);
    void setTextFilter(const QString &text);
    void setFileView(bool fileView);
    /** Uppercase extensions without dots; empty = no type filter. */
    void setExtFilter(const QStringList &exts);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    bool hideFileLists_;
    bool requireFullFile_;
    bool fullOnly_;
    bool fileView_;
    QString textFilter_;
    QStringList extFilter_;
};

bool isFinishedFileList(const std::string &path);
