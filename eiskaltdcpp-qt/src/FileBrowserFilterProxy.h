/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include "FileBrowserModel.h"

#include <QSortFilterProxyModel>
#include <QStringList>
#include <string>
#include <vector>

namespace dcpp {
class DirectoryListing;
}

/** Filter proxy that keeps FileBrowserModel::sort and applies Search-like view filters. */
class FileBrowserFilterProxy : public QSortFilterProxyModel {
Q_OBJECT
public:
    explicit FileBrowserFilterProxy(bool treeMode = false, QObject *parent = nullptr);

    void sort(int column, Qt::SortOrder order) override;

    void applyFilters(const QStringList &terms, qulonglong size, int sizeMode,
                      bool dirsOnly, bool filesOnly, const QStringList &exts,
                      const QString &pathPrefix);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    struct Term {
        std::string value;
        bool exclude = false;
    };

    bool filtersActive() const;
    bool matchesText(const std::string &haystack) const;
    bool filePasses(const QString &filePath, qulonglong size, const QString &tth) const;
    bool dirPasses(const QString &path, qulonglong size) const;
    bool subtreeHasMatch(dcpp::DirectoryListing::Directory *dir, const QString &path) const;
    bool subtreeHasVisibleDir(dcpp::DirectoryListing::Directory *dir, const QString &path) const;

    QStringList textTermsRaw_;
    std::vector<Term> textTerms_;
    qulonglong sizeLimit_ = 0;
    int sizeMode_ = 0;
    bool dirsOnly_ = false;
    bool filesOnly_ = false;
    QStringList extFilter_;
    QString pathPrefix_;
    bool treeMode_ = false;
};
