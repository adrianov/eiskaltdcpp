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
#include "WulforUtil.h"

#include "dcpp/SettingsManager.h"
#include "dcpp/SearchResult.h"

using namespace dcpp;

void SearchFrame::getParams(SearchFrame::VarMap &map, const dcpp::SearchResultPtr &ptr){
    map.clear();

    map["SIZE"]    = qulonglong(ptr->getSize());

    QString fname = _q(ptr->getFile());
    const QStringList &fname_parts = fname.split('\\', WULFOR_SKIP_EMPTY);

    map["FILE"] = fname_parts.isEmpty()? fname : fname_parts.last();

    if (ptr->getType() == SearchResult::TYPE_FILE){
        map["TTH"]  = _q(ptr->getTTH().toBase32());

        QString path = fname.left(fname.lastIndexOf("\\"));

        if (!path.endsWith("\\"))
            path += "\\";

        map["PATH"] = path;
        map["ISDIR"]   = false;
    }
    else{
        map["PATH"] = _q(ptr->getFile()).left(_q(ptr->getFile()).lastIndexOf(map["FILE"].toString()));
        map["TTH"]  = "";
        map["ISDIR"] = true;
    }

    map["NICK"]    = WulforUtil::getInstance()->getNicks(ptr->getUser()->getCID());
    map["FSLS"]    = ptr->getFreeSlots();
    map["ASLS"]    = ptr->getSlots();
    map["IP"]      = _q(ptr->getIP());
    map["HUB"]     = _q(ptr->getHubName());
    map["HOST"]    = _q(ptr->getHubURL());
    map["CID"]     = _q(ptr->getUser()->getCID().toBase32());
}

bool SearchFrame::getDownloadParams(SearchFrame::VarMap &params, SearchItem *item){
    if (!item)
        return false;

    params.clear();

    QString fname = item->data(COLUMN_SF_PATH).toString() + item->data(COLUMN_SF_FILENAME).toString();

    if (item->isDir && !fname.endsWith('\\'))
        fname += "\\";

    params["CID"]   = item->cid;
    params["FNAME"] = fname;
    params["ESIZE"] = item->data(COLUMN_SF_ESIZE);
    params["TTH"]   = item->data(COLUMN_SF_TTH);
    params["HOST"]  = item->data(COLUMN_SF_HOST);
    params["TARGET"]= _q(SETTING(DOWNLOAD_DIRECTORY));

    return true;
}

bool SearchFrame::getWholeDirParams(SearchFrame::VarMap &params, SearchItem *item){
    if (!item)
        return false;

    params.clear();

    params["CID"]   = item->cid;

    if (item->isDir)
        params["FNAME"] = item->data(COLUMN_SF_PATH).toString() + item->data(COLUMN_SF_FILENAME).toString();
    else
        params["FNAME"] = item->data(COLUMN_SF_PATH).toString();//Download directory that containing a file

    params["ESIZE"] = 0;
    params["TTH"]   = "";
    params["HOST"]  = item->data(COLUMN_SF_HOST);
    params["TARGET"]= _q(SETTING(DOWNLOAD_DIRECTORY));

    return true;
}

void SearchFrame::addResult(const QVariantMap &map){
    addResults(QList<VarMap>() << map);
}

void SearchFrame::addResultsPacked(const QVariant &packed){
    addResults(packed.value<QList<VarMap> >());
}

void SearchFrame::queueResult(const VarMap &map){
    Q_D(SearchFrame);
    d->pendingResults.append(map);
    if (d->resultFlush && !d->resultFlush->isActive())
        d->resultFlush->start();
}

void SearchFrame::flushResults(){
    Q_D(SearchFrame);
    if (d->pendingResults.isEmpty())
        return;
    const QList<VarMap> batch = d->pendingResults;
    d->pendingResults.clear();
    addResults(batch);
}

void SearchFrame::addResults(const QList<VarMap> &maps){
    Q_D(SearchFrame);
    static SearchBlacklist *SB = SearchBlacklist::getInstance();

    for (const VarMap &map : maps) {
        try {
            if (SB->ok(map["FILE"].toString(), SearchBlacklist::NAME) && SB->ok(map["TTH"].toString(), SearchBlacklist::TTH)){
                if (d->model->addResult(map["FILE"].toString(),
                                        map["SIZE"].toULongLong(),
                                        map["TTH"].toString(),
                                        map["PATH"].toString(),
                                        map["NICK"].toString(),
                                        map["FSLS"].toULongLong(),
                                        map["ASLS"].toULongLong(),
                                        map["IP"].toString(),
                                        map["HUB"].toString(),
                                        map["HOST"].toString(),
                                        map["CID"].toString(),
                                        map["ISDIR"].toBool()))
                    d->results++;
            }
        }
        catch (const SearchListException&){}
    }
}
