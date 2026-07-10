/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

#include "dcpp/stdinc.h"
#include "dcpp/Singleton.h"

/** Exact-match filter for private-message bodies marked as spam from the sidebar. */
class PmSpamFilter:
        public QObject,
        public dcpp::Singleton<PmSpamFilter>
{
    Q_OBJECT

    friend class dcpp::Singleton<PmSpamFilter>;

public:
    bool isSpam(const QString &msg) const;
    void add(const QString &msg);
    void addAll(const QStringList &msgs);

private:
    PmSpamFilter();
    virtual ~PmSpamFilter();

    static QString normalize(const QString &msg);

    void load();
    void save() const;

    QSet<QString> texts;
};
