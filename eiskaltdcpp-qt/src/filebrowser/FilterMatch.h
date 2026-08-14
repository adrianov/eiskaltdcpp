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

#include <QString>
#include <QStringList>
#include <vector>

/** Copyable file-list filter criteria and file/dir predicates. */
class FilterMatch
{
public:
    struct Term {
        QString value;
        bool exclude = false;
        bool operator==(const Term &o) const { return value == o.value && exclude == o.exclude; }
    };

    std::vector<Term> terms;
    qulonglong sizeLimit = 0;
    int sizeMode = 0;
    bool dirsOnly = false;
    bool filesOnly = false;
    bool adultVideo = false;
    QStringList extFilter;

    bool operator==(const FilterMatch &o) const {
        return terms == o.terms && sizeLimit == o.sizeLimit && sizeMode == o.sizeMode
                && dirsOnly == o.dirsOnly && filesOnly == o.filesOnly
                && adultVideo == o.adultVideo && extFilter == o.extFilter;
    }

    static QString joinPath(const QString &path, const QString &name) {
        if (path.isEmpty())
            return name;
        const QChar last = path.at(path.size() - 1);
        if (last == QLatin1Char('\\') || last == QLatin1Char('/'))
            return path + name;
        return path + QLatin1Char('\\') + name;
    }

    bool needTth() const;
    bool matchesText(const QString &haystack) const;
    bool sizeOk(qulonglong size) const;
    bool dirPasses(const QString &path, qulonglong size) const;
    bool acceptFile(const QString &name, const QString &path, const QString &tth,
                    qulonglong size) const;
    bool isActive() const;
    void setTerms(const QStringList &raw);
};
