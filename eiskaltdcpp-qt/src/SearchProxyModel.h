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
#include <QStringList>
#include <string>
#include <vector>

/** Live view filter for Search results (text terms, size, file type). */
class SearchProxyModel: public QSortFilterProxyModel {
Q_OBJECT
public:
    explicit SearchProxyModel(QObject *parent = nullptr);

    void sort(int column, Qt::SortOrder order) override;

    /** Update predicates; invalidate only when something changed. */
    void applyFilters(const QStringList &terms, qulonglong size, int sizeMode,
                      bool dirsOnly, bool filesOnly, const QStringList &exts);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    struct Term {
        std::string value;
        bool exclude = false;
    };

    QStringList textTermsRaw_;
    std::vector<Term> textTerms_;
    qulonglong sizeLimit_ = 0;
    int sizeMode_ = 0;
    bool dirsOnly_ = false;
    bool filesOnly_ = false;
    QStringList extFilter_;
};
