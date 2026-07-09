/***************************************************************************
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
#include <QList>
#include <QCompleter>
#include <QMenu>
#include <QShortcut>

#include "SearchFrame.h"
#include "SearchModel.h"

#include "dcpp/stdinc.h"
#include "dcpp/Client.h"
#include "dcpp/typedefs.h"

class SearchFramePrivate {
public:
    QString arena_title;
    QString token;

    TStringList currentSearch;

    qulonglong dropped = 0;
    qulonglong results = 0;
    SearchFrame::AlreadySharedAction filterShared = SearchFrame::None;
    bool withFreeSlots = false;

    QStringList hubs;
    QStringList searchHistory;

    QList<dcpp::Client*> client_list;

    QCompleter *completer = nullptr;

    QMenu *arena_menu = nullptr;

    QShortcut *focusShortcut = nullptr;

    bool saveFileType = true;

    SearchModel *model = nullptr;
    SearchStringListModel *str_model = nullptr;
    SearchProxyModel *proxy = nullptr;

    bool isHash = false;
    bool stop = false;
    int left_pane_old_size = 0;

    QString target;
    uint64_t searchStartTime = 0;
    uint64_t searchEndTime = 0;
    bool waitingResults = false;
    int indexStatsTick = 0;
};
