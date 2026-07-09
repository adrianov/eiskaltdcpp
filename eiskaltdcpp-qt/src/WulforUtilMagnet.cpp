/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "WulforUtil.h"

#include "dcpp/ClientManager.h"
#include "dcpp/User.h"
#include "dcpp/CID.h"
#include "dcpp/Util.h"
#include "extra/magnet.h"

using namespace dcpp;

QString WulforUtil::makeMagnet(const QString &path, const int64_t size, const QString &tth){
    if (path.isEmpty() || tth.isEmpty())
        return QString();

    return magnetSignature + tth + "&xl=" + _q(Util::toString(size)) + "&dn=" + _q(Util::encodeURI(path.toStdString()));
}

void WulforUtil::splitMagnet(const QString &magnet, int64_t &size, QString &tth, QString &name){
    size = 0;
    tth = "";
    name = "";

    StringMap params;
    if (magnet::parseUri(magnet.toStdString(),params)) {
        // Name of file or directory (or search keywords if name is not set)
        name = _q(params["dn"]);
        if (name.isEmpty() && !params["kt"].empty()) {
            name = _q(params["kt"]);
        }

        // BitTorrent magnet links are quite popular nowadays...
        if (!magnet.contains("urn:btih:") && !magnet.contains("urn:btmh:")) {
            tth = _q(params["xt"]);
        }

        // Size of file or directory
        if (!params["xl"].empty()) {
            size = Util::toInt64(params["xl"]);
        }
        if (!params["dl"].empty()) { // this size is more valuable if it is set
            size = Util::toInt64(params["dl"]);
        }
    }
}

int WulforUtil::sortOrderToInt(Qt::SortOrder order){
    if (order == Qt::AscendingOrder)
        return 0;
    else
        return 1;
}

Qt::SortOrder WulforUtil::intToSortOrder(int i){
    if (!i)
        return Qt::AscendingOrder;
    else
        return Qt::DescendingOrder;
}

QString WulforUtil::getHubNames(const dcpp::CID &cid){
    StringList hubs = ClientManager::getInstance()->getHubNames(cid, "");

    if (hubs.empty())
        return tr("Offline");
    else
        return _q(Util::toString(hubs));
}

QString WulforUtil::getHubNames(const dcpp::UserPtr &user){
    return getHubNames(user->getCID());
}

QString WulforUtil::getHubNames(const QString &cid){
    return getHubNames(CID(_tq(cid)));
}

QString WulforUtil::compactToolTipText(QString text, int maxlen, QString sep)
{
    int len = text.size();

    if (len <= maxlen)
        return text;

    int n = 0;
    int k = maxlen;

    while((len-k) > 0){
        if(text.at(k) == ' ' || (k == n))
        {
            if(k == n)
                k += maxlen;

            text.insert(k+1,sep);

            len++;
            k += maxlen + 1;
            n += maxlen + 1;
        }
        else k--;
    }

    return text;
}

