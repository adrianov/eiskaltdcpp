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
#include "SearchBlacklist.h"
#include "SearchBlacklistDialog.h"
#include "WulforUtil.h"
#include "HubFrame.h"
#include "HubManager.h"

#include "dcpp/FavoriteManager.h"
#include "dcpp/UserCommand.h"
#include "dcpp/ClientManager.h"

using namespace dcpp;

bool SearchFrame::contextUserActions(Menu::Action act, const QModelIndexList &list)
{
    Q_D(SearchFrame);
    QItemSelectionModel *selection_model = treeView_RESULTS->selectionModel();
    switch (act){
        case Menu::SendPM:
        {
            HubFrame *fr = nullptr;

            for (const auto &i : list){
                SearchItem *item = reinterpret_cast<SearchItem*>(i.internalPointer());

                QString hubUrl = item->data(COLUMN_SF_HOST).toString();
                dcpp::CID cid(_tq(item->cid));

                fr = qobject_cast<HubFrame*>(HubManager::getInstance()->getHub(hubUrl));

                if (fr)
                    fr->createPMWindow(cid);
            }

            break;
        }
        case Menu::AddToFav:
        {
            for (const auto &i : list){
                SearchItem *item = reinterpret_cast<SearchItem*>(i.internalPointer());
                VarMap params;

                if (getDownloadParams(params, item))
                    addToFav(params["CID"].toString());

            }

            break;
        }
        case Menu::GrantExtraSlot:
        {
             for (const auto &i : list){
                SearchItem *item = reinterpret_cast<SearchItem*>(i.internalPointer());
                VarMap params;

                if (getDownloadParams(params, item))
                    grant(params);

            }

            break;
        }
        case Menu::RemoveFromQueue:
        {
             for (const auto &i : list){
                SearchItem *item = reinterpret_cast<SearchItem*>(i.internalPointer());
                VarMap params;

                if (getDownloadParams(params, item))
                    removeSource(params);

             }

             break;
        }
        case Menu::Remove:
        {
             selection_model->clearSelection();

             for (const auto &i : list){
                SearchItem *item = reinterpret_cast<SearchItem*>(i.internalPointer());

                d->model->removeItem(item);

                d->model->repaint();
            }

            break;
        }
        case Menu::UserCommands:
        {
            for (const auto &i : list){
                SearchItem *item = reinterpret_cast<SearchItem*>(i.internalPointer());

                int id = Menu::getInstance()->getCommandId();

                UserCommand uc;
                if (id == -1 || !FavoriteManager::getInstance()->getUserCommand(id, uc))
                    break;

                StringMap params;

                if (WulforUtil::getInstance()->getUserCommandParams(uc, params)){
                    UserPtr user = ClientManager::getInstance()->findUser(CID(item->cid.toStdString()));

                    if (user && user->isOnline()){
                        params["fileFN"]     = _tq(item->data(COLUMN_SF_PATH).toString() + item->data(COLUMN_SF_FILENAME).toString());
                        params["fileSI"]     = _tq(item->data(COLUMN_SF_ESIZE).toString());
                        params["fileSIshort"]= _tq(item->data(COLUMN_SF_SIZE).toString());

                        if(!item->isDir)
                            params["fileTR"] = _tq(item->data(COLUMN_SF_TTH).toString());

                        // compatibility with 0.674 and earlier
                        params["file"] = params["fileFN"];
                        params["filesize"] = params["fileSI"];
                        params["filesizeshort"] = params["fileSIshort"];
                        params["tth"] = params["fileTR"];

                        string hubUrl = _tq(i.data(COLUMN_SF_HOST).toString());

                        ClientManager::getInstance()->userCommand(HintedUser(user, hubUrl), uc, params, true);
                    }

                }
            }

            break;
        }
        case Menu::Blacklist:
        {
            SearchBlackListDialog dlg(this);

            dlg.exec();

            break;
        }
        case Menu::AddToBlacklist:
        {
            QList <QString> new_inst;

            for (const auto &i : list){
                SearchItem *item = reinterpret_cast<SearchItem*>(i.internalPointer());
                VarMap params;

                if (getDownloadParams(params, item))
                    new_inst << item->data(COLUMN_SF_FILENAME).toString();
            }
            if (!new_inst.isEmpty()){
                static SearchBlacklist *SB = SearchBlacklist::getInstance();
                QList <QString> list = SB->getList(SearchBlacklist::NAME);
                list.append(new_inst);
                SB->setList(SearchBlacklist::NAME, list);
            }

            break;
        }
        default:
            return false;
    }
    return true;
}
