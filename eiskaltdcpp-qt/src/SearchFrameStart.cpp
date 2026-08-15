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

#include "SearchFrame.h"
#include "SearchFramePrivate.h"
#include "SearchFrameLocal.h"
#include "MainWindow.h"
#include "WulforUtil.h"
#include "GlobalTimer.h"

#include "dcpp/StringTokenizer.h"
#include "dcpp/SettingsManager.h"
#include "dcpp/SearchManager.h"
#include "dcpp/ClientManager.h"
#include "dcpp/Util.h"

#include <QPushButton>

using namespace dcpp;

namespace {

StringList checkedHubUrls(SearchStringListModel *model)
{
    StringList clients;
    for (int i = 0; i < model->rowCount(); ++i) {
        const QModelIndex index = model->index(i, 0);
        if (index.data(Qt::CheckStateRole).toInt() == Qt::Checked)
            clients.push_back(_tq(index.data().toString()));
    }
    return clients;
}

QStringList compactQuery(TStringList &tokens, QString &joined)
{
    QStringList terms;
    joined.clear();
    for (auto si = tokens.begin(); si != tokens.end(); ) {
        if (si->empty()) {
            si = tokens.erase(si);
            continue;
        }
        if ((*si)[0] != '-') {
            const QString term = QString::fromStdString(*si);
            terms << term;
            joined += term + QLatin1Char(' ');
        }
        ++si;
    }
    return terms;
}

StringList typeExtensions(int &ftype, const QString &typeName)
{
    string ftypeStr;
    if (ftype > SearchManager::TYPE_ANY && ftype < SearchManager::TYPE_LAST)
        ftypeStr = SearchManager::getTypeStr(ftype);
    else if (ftype >= SearchManager::TYPE_LAST && !typeName.isEmpty())
        ftypeStr = _tq(typeName);
    else
        ftype = SearchManager::TYPE_ANY;

    try {
        if (ftype == SearchManager::TYPE_ANY && !ftypeStr.empty())
            return SettingsManager::getInstance()->getExtensions(ftypeStr);
        if ((ftype > SearchManager::TYPE_ANY && ftype < SearchManager::TYPE_DIRECTORY) ||
            ftype == SearchManager::TYPE_CD_IMAGE ||
            ftype == SearchManager::TYPE_AUDIO_VIDEO)
            return SearchManager::getTypeExtensions(ftype);
    } catch (const SearchTypeException&) {
        ftype = SearchManager::TYPE_ANY;
    }
    return StringList();
}

QStringList localExtList(const StringList &exts)
{
    QStringList localExts;
    for (const auto &e : exts) {
        QString s = _q(e).trimmed().toUpper();
        if (s.startsWith(QLatin1Char('.')))
            s = s.mid(1);
        if (!s.isEmpty())
            localExts << s;
    }
    return localExts;
}

} // namespace

void SearchFrame::slotStartSearch(){
    if (qobject_cast<QPushButton*>(sender()) != pushButton_SEARCH){
        pushButton_SEARCH->click(); //Generating clicked() signal that shows pushButton_STOP button.
                                    //Anybody can suggest something better?
        return;
    }

    Q_D(SearchFrame);

    // Drop prior hub queries from this tab before a new one.
    ClientManager::getInstance()->cancelSearch((void*)this);

    d->stop = false;

    QString s = lineEdit_SEARCHSTR->text().trimmed();
    if (s.isEmpty())
        return;

    StringList clients = checkedHubUrls(d->str_model);
#ifndef WITH_DHT
    if (clients.empty())
        return;
#endif

    quint64 llsize = 0;
    int sizeModeInt = SearchManager::SIZE_DONTCARE;
    readSizeFilter(llsize, sizeModeInt);
    const SearchManager::SizeModes searchMode = static_cast<SearchManager::SizeModes>(sizeModeInt);

    rememberSearch(s);

    d->currentSearch = StringTokenizer<string>(s.toStdString(), ' ').getTokens();
    const QStringList terms = compactQuery(d->currentSearch, s);
    d->token = _q(Util::toString(Util::rand()));

    const int idx = comboBox_FILETYPES->currentIndex();
    int ftype = (idx >= 0) ? comboBox_FILETYPES->itemData(idx).toInt() : SearchManager::TYPE_ANY;
    const QString typeName = (idx >= 0) ? comboBox_FILETYPES->itemText(idx) : QString();

    d->isHash = (ftype == SearchManager::TYPE_TTH);
    d->filterShared = static_cast<AlreadySharedAction>(comboBox_SHARED->currentIndex());
    d->withFreeSlots = checkBox_FILTERSLOTS->isChecked();
    d->model->setFilterRole(static_cast<int>(d->filterShared));

    resetResultState();
    d->viewFilterPending = false;
    applyViewFiltersNow();

    const StringList exts = typeExtensions(ftype, typeName);
    d->target = s;
    d->searchStartTime = GlobalTimer::getInstance()->getTicks()*1000;

    const bool dirsOnly = (ftype == SearchManager::TYPE_DIRECTORY);
    const bool filesOnly = (ftype != SearchManager::TYPE_ANY && ftype != SearchManager::TYPE_DIRECTORY);
    SearchFrameLocal::startLocalSearch(this, terms, d->isHash, dirsOnly, filesOnly,
                                      qint64(llsize), int(searchMode), localExtList(exts));

    const uint64_t maxDelayBeforeSearch = SearchManager::getInstance()->search(clients, s.toStdString(), llsize, SearchManager::TypeModes(ftype), searchMode, d->token.toStdString(), exts, (void*)this);
    d->searchEndTime = d->searchStartTime + maxDelayBeforeSearch + 20000; // most hub hits arrive within ~20s
    d->waitingResults = true;

    if (!checkBox_HIDEPANEL->isChecked()){
        QList<int> panes = splitter->sizes();

        panes[1] = panes[0] + panes[1];

        d->left_pane_old_size = panes[0] > 15 ? panes[0] : d->left_pane_old_size;

        panes[0] = 0;

        splitter->setSizes(panes);
    }

    d->arena_title = tr("Search - %1").arg(s);

    MainWindow::getInstance()->redrawToolPanel();
}
