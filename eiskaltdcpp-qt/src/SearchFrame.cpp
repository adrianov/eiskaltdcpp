/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include <QComboBox>
#include <QTreeView>
#include <QAction>
#include <QTextCodec>
#include <QDir>
#include <QItemSelectionModel>
#include <QFileDialog>
#include <QClipboard>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMessageBox>
#include <QStringListModel>
#include <QKeyEvent>
#include <QTimer>
#include <QShortcut>
#include <QStringListModel>
#include <QCompleter>
#include <QListWidget>
#include <QListWidgetItem>

#include "SearchFrame.h"
#include "SearchFramePrivate.h"
#include "SearchFrameLocal.h"
#include "MainWindow.h"
#include "HubFrame.h"
#include "HubManager.h"
#include "SearchModel.h"
#include "SearchBlacklist.h"
#include "SearchBlacklistDialog.h"
#include "WulforUtil.h"
#include "Magnet.h"
#include "ArenaWidgetManager.h"
#include "ArenaWidgetFactory.h"
#include "DownloadToHistory.h"
#include "GlobalTimer.h"

#include "dcpp/CID.h"
#include "dcpp/ClientManager.h"
#include "dcpp/FavoriteManager.h"
#include "dcpp/StringTokenizer.h"
#include "dcpp/SettingsManager.h"
#include "dcpp/Encoder.h"
#include "dcpp/UserCommand.h"

#include <QtDebug>

using namespace dcpp;

QVariant SearchStringListModel::data(const QModelIndex &index, int role) const{
    if (role != Qt::CheckStateRole || !index.isValid())
        return QStringListModel::data(index, role);

    return (checked.contains(index.data().toString())? Qt::Checked : Qt::Unchecked);
}

bool SearchStringListModel::setData(const QModelIndex &index, const QVariant &value, int role){
    if (role != Qt::CheckStateRole || !index.isValid())
        return QStringListModel::setData(index, value, role);

    if (value.toInt() == Qt::Checked)
        checked.push_back(index.data().toString());
    else if (checked.contains(index.data().toString()))
        checked.removeAt(checked.indexOf(index.data().toString()));

    return true;
}

