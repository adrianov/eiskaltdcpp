/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "PublicHubs.h"

#include <QSet>
#include <QStringList>

using namespace dcpp;

void PublicHubs::fillCountries(){
    const QString prev = comboBox_COUNTRY->currentData().toString();

    comboBox_COUNTRY->blockSignals(true);
    comboBox_COUNTRY->clear();
    comboBox_COUNTRY->addItem(tr("All countries"), QString());

    QSet<QString> countries;
    for (const auto &e : entries) {
        const QString c = _q(e.getCountry()).trimmed();
        if (!c.isEmpty())
            countries.insert(c);
    }

    QStringList list = countries.values();
    list.sort(Qt::CaseInsensitive);
    for (const QString &c : list)
        comboBox_COUNTRY->addItem(c, c);

    const int idx = comboBox_COUNTRY->findData(prev);
    comboBox_COUNTRY->setCurrentIndex(idx >= 0 ? idx : 0);
    comboBox_COUNTRY->blockSignals(false);
}

void PublicHubs::updateList(){
    if (!model)
        return;

    const QString country = comboBox_COUNTRY->currentData().toString();

    model->clearModel();
    QList<QVariant> data;

    for (auto &i : entries) {
        HubEntry *entry = const_cast<HubEntry*>(&i);

        if (!country.isEmpty() && _q(entry->getCountry()) != country)
            continue;

        data.clear();
        data << _q(entry->getName())         << _q(entry->getDescription())  << entry->getUsers()
             << _q(entry->getServer())       << _q(entry->getCountry())      << (qlonglong)entry->getShared()
             << (qint64)entry->getMinShare() << (qint64)entry->getMinSlots() << (qint64)entry->getMaxHubs()
             << (qint64)entry->getMaxUsers() << static_cast<double>(entry->getReliability()) << _q(entry->getRating());

        model->addResult(data, entry);
    }

    int col = treeView->header()->sortIndicatorSection();
    Qt::SortOrder order = treeView->header()->sortIndicatorOrder();
    if (col < 0) {
        col = COLUMN_PHUB_USERS;
        order = Qt::DescendingOrder;
    }
    treeView->sortByColumn(col, order);
}

void PublicHubs::slotCountryChanged(int){
    updateList();
}
