/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "WulforUtil.h"
#include "TransferDisplay.h"

#include "dcpp/ClientManager.h"
#include "dcpp/User.h"
#include "dcpp/CID.h"
#include "dcpp/Util.h"

#include <QTextCodec>
#include <QAction>
#include <QRegExp>

using namespace dcpp;

QString WulforUtil::getNicks(const QString &cid, const QString &hintUrl){
    return getNicks(CID(cid.toStdString()), hintUrl);
}

QString WulforUtil::getNickViaOnlineUser(const QString &cid, const QString &hintUrl) {
    return _q(ClientManager::getInstance()->getField(CID(_tq(cid)), _tq(hintUrl), "NI"));
}

QString WulforUtil::getNicks(const CID &cid, const QString &hintUrl){
    return _q(dcpp::Util::toString(ClientManager::getInstance()->getNicks(cid, _tq(hintUrl))));
}

void WulforUtil::textToHtml(QString &str, bool print){
    if (print){
        str.replace(";", "&#59;");
        str.replace("<", "&lt;");
        str.replace(">", "&gt;");
    }
    else {
        str.replace("\n", "<br/>");
        str.replace("\t", "&nbsp;&nbsp;&nbsp;&nbsp;");
    }
}

QString WulforUtil::qtEnc2DcEnc(QString name){
    if (QtEnc2DCEnc.contains(name))
        return QtEnc2DCEnc[name].left(QtEnc2DCEnc[name].indexOf(" "));
    else
        return "";
}

QString WulforUtil::dcEnc2QtEnc(QString name){
    for (auto it = QtEnc2DCEnc.begin(); it != QtEnc2DCEnc.end(); ++it){
        if (it.value() == name || !it.value().indexOf(name))
            return it.key();
    }

    return tr("System default");
}

QStringList WulforUtil::encodings(){
    return QtEnc2DCEnc.keys();
}

QTextCodec *WulforUtil::codecForEncoding(const QString &name){
    if (!QtEnc2DCEnc.contains(name))
        return QTextCodec::codecForLocale();

    return QTextCodec::codecForName(name.toUtf8());
}

QString WulforUtil::formatBytes(int64_t aBytes){
    return _q(Util::formatBytes(aBytes));
}

QString WulforUtil::formatDisplayBytes(int64_t aBytes){
    return formatBytes(static_cast<int64_t>(TransferDisplay::roundBytes(aBytes)));
}

void WulforUtil::bindActionIcon(QAction *act, Icons icon)
{
    act->setProperty("wulforIcon", icon);
    act->setIcon(getInstance()->getIcon(icon));
}

bool WulforUtil::isTTH ( const QString& text ) {
    return ((text.length() == 39) && (QRegExp("[A-Z0-9]+").exactMatch(text)));
}
