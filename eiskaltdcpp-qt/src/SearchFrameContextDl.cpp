/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SearchFrame.h"
#include "SearchFramePrivate.h"
#include "SearchModel.h"
#include "WulforUtil.h"
#include "DownloadToHistory.h"
#include "ArenaWidgetFactory.h"
#include "HubManager.h"
#include "HubFrame.h"

#include "dcpp/ClientManager.h"
#include "dcpp/SettingsManager.h"

#include <QFileDialog>
#include <QDir>
#include <QClipboard>
#include <QApplication>

#include "Magnet.h"

using namespace dcpp;

bool SearchFrame::contextDownloads(Menu::Action act, const QModelIndexList &list)
{
    switch (act){
        case Menu::Download:
        {
            for (const auto &i : list){
                SearchItem *item = reinterpret_cast<SearchItem*>(i.internalPointer());
                VarMap params;

                if (getDownloadParams(params, item)){
                    download(params);

                    if (item->childCount() > 0 && !SETTING(DONT_DL_ALREADY_QUEUED)){//download all child items
                        QString fname = params["FNAME"].toString();

                        for (const auto &i : item->childItems){
                            if (getDownloadParams(params, i)){
                                params["FNAME"] = fname;

                                download(params);
                            }
                        }
                    }
                }
            }

            break;
        }
        case Menu::DownloadTo:
        {
            static QString old_target = QDir::homePath();
            QString target = Menu::getInstance()->getDownloadToPath();

            if (!QDir(target).exists() || target.isEmpty()){
                target = QFileDialog::getExistingDirectory(this, tr("Select directory"), old_target);

                target = QDir::toNativeSeparators(target);

                Menu::getInstance()->addTempPath(target);
            }

            if (target.isEmpty())
                break;

            if (!target.endsWith(QDir::separator()))
                target += QDir::separator();

            old_target = target;

            for (const auto &i : list){
                SearchItem *item = reinterpret_cast<SearchItem*>(i.internalPointer());
                VarMap params;

                if (getDownloadParams(params, item)){
                    params["TARGET"] = target;
                    download(params);

                    if (item->childCount() > 0 && !SETTING(DONT_DL_ALREADY_QUEUED)){//download all child items
                        QString fname = params["FNAME"].toString();

                        for (const auto  &i : item->childItems){
                            if (getDownloadParams(params, i)){
                                params["FNAME"]  = fname;
                                params["TARGET"] = target;

                                download(params);
                            }
                        }
                    }
                }
            }

            break;
        }
        case Menu::DownloadWholeDir:
        {
            for (const auto &i : list){
                SearchItem *item = reinterpret_cast<SearchItem*>(i.internalPointer());
                VarMap params;

                if (getWholeDirParams(params, item))
                    download(params);
            }

            break;
        }
        case Menu::DownloadWholeDirTo:
        {
            static QString old_target = QDir::homePath();
            QString target = Menu::getInstance()->getDownloadToPath();

            if (!QDir(target).exists() || target.isEmpty()){
                target = QFileDialog::getExistingDirectory(this, tr("Select directory"), old_target);

                target = QDir::toNativeSeparators(target);

                Menu::getInstance()->addTempPath(target);
            }

            if (target.isEmpty())
                break;

            if (!target.endsWith(QDir::separator()))
                target += QDir::separator();

            old_target = target;

            for (const auto &i : list){
                SearchItem *item = reinterpret_cast<SearchItem*>(i.internalPointer());
                VarMap params;

                if (getWholeDirParams(params, item)){
                    params["TARGET"] = target;
                    download(params);
                }
            }

            break;
        }
        default:
            return false;
    }
    return true;
}
