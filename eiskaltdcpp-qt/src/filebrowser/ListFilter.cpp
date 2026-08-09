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

#include "filebrowser/ListFilter.h"
#include "FileBrowserModel.h"
#include "sharebrowser/AsyncRunner.h"

#include <QMetaObject>
#include <QPointer>
#include <QThread>

bool ListFilter::set(const QStringList &terms, qulonglong size, int sizeMode,
                     bool dirsOnly, bool filesOnly, const QStringList &exts)
{
    QStringList upperExts;
    upperExts.reserve(exts.size());
    for (const QString &ext : exts)
        upperExts.append(ext.toUpper());

    if (textTermsRaw_ == terms && match_.sizeLimit == size && match_.sizeMode == sizeMode
            && match_.dirsOnly == dirsOnly && match_.filesOnly == filesOnly
            && match_.extFilter == upperExts)
        return false;

    if (textTermsRaw_ != terms) {
        textTermsRaw_ = terms;
        match_.setTerms(terms);
    }

    match_.sizeLimit = size;
    match_.sizeMode = sizeMode;
    match_.dirsOnly = dirsOnly;
    match_.filesOnly = filesOnly;
    match_.extFilter = upperExts;
    ++(*gen_);
    return true;
}

void ListFilter::cancel()
{
    ++(*gen_);
}

void ListFilter::scanAsync(FileBrowserItem *root, const QString &pathPrefix, QObject *receiver,
                           const std::function<void(QVector<int>)> &onDone)
{
    if (!root || !receiver || !onDone)
        return;

    const std::shared_ptr<std::atomic<int>> liveGen = gen_;
    const int gen = liveGen->load();
    const FilterMatch match = match_;
    // QList COW: O(1) on the UI thread; item fields are read on the worker.
    const QList<FileBrowserItem*> items = root->childItems;

    QPointer<QObject> guard(receiver);
    // Parent to receiver so ListFilterProxy can wait for us before items are freed.
    AsyncRunner *runner = new AsyncRunner(receiver);
    runner->setRunFunction([guard, liveGen, gen, items, match, pathPrefix, onDone]() {
        QVector<int> rows;
        rows.reserve(qMin(items.size(), 4096));
        for (int i = 0; i < items.size(); ++i) {
            if ((i & 63) == 0 && liveGen->load() != gen)
                return;
            if (match.acceptItem(items.at(i), pathPrefix))
                rows.append(i);
        }
        if (!guard || liveGen->load() != gen)
            return;
        QMetaObject::invokeMethod(guard.data(), [guard, liveGen, gen, rows, onDone]() {
            if (!guard || liveGen->load() != gen)
                return;
            onDone(rows);
        }, Qt::QueuedConnection);
    });
    QObject::connect(runner, &QThread::finished, runner, &QObject::deleteLater);
    runner->start();
}
