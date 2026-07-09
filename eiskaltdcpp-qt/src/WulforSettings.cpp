/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "WulforSettings.h"
#include "WulforUtil.h"
#include "AppTheme.h"

#include "dcpp/stdinc.h"
#include "dcpp/File.h"
#include "dcpp/Util.h"
#include "dcpp/SimpleXML.h"

#include <QApplication>
#include <QFile>
#include <QUrl>
#include <QtDebug>

using namespace dcpp;

WulforSettings::WulforSettings()
    : settings(_q(Util::getPath(Util::PATH_USER_CONFIG)) + "EiskaltDC++_Qt.conf", QSettings::IniFormat)
    , configFileOld(_q(Util::getPath(Util::PATH_USER_CONFIG)) + "EiskaltDC++.xml")
    , appTranslator(nullptr)
    , qtTranslator(nullptr)
    , qtBaseTranslator(nullptr)
{
    QStringList idns = QUrl::idnWhitelist();
    idns.push_back("рф");
    QUrl::setIdnWhitelist(idns);

    connect(this, SIGNAL(fontChanged(QString,QString)),
            this, SIGNAL(strValueChanged(QString,QString)));
}

WulforSettings::~WulforSettings(){
}

bool WulforSettings::hasKey(const QString &key) const{
    return (intmap.keys().contains(key) || strmap.keys().contains(key));
}

void WulforSettings::load(){
#ifdef _DEBUG_QT_UI
    qDebug() << settings.fileName();
#endif
    if (QFile::exists(configFileOld) && settings.value("app/firstrun", true).toBool()){
        loadOldConfig();

        //And load old config into QSettings
        for (auto it = strmap.begin(); it != strmap.end(); ++it) {
            settings.setValue(it.key(), it.value());
        }

        for (auto iit = intmap.begin(); iit != intmap.end(); ++iit) {
            settings.setValue(iit.key(), iit.value());
        }

        // QFile(configFileOld).remove();

        intmap.clear();
        strmap.clear();

        settings.setValue("app/firstrun", false);
    }

    if (settings.value("app/firstrun", true).toBool()){
        settings.setValue("app/firstrun", false);
        writeFirstRunDefaults();
    }

    if (!settings.value("app/statusbar-history-migrated", false).toBool()) {
        if (settings.value(WI_STATUSBAR_HISTORY_SZ).toInt() == 5)
            settings.setValue(WI_STATUSBAR_HISTORY_SZ, WI_STATUSBAR_HISTORY_DEFAULT);
        settings.setValue("app/statusbar-history-migrated", true);
    }
}

void WulforSettings::loadOldConfig(){
    fillOldConfigMaps();

    if (!QFile::exists(configFileOld))
        return;

    try{
        SimpleXML xml;
        xml.fromXML(File(configFileOld.toStdString(), File::READ, File::OPEN).read());
        xml.resetCurrentChild();
        xml.stepIn();

        if (xml.findChild("Settings")){
            xml.stepIn();

            for (const QString &key : strmap.keys()) {
                if (xml.findChild(key.toStdString())) {
                        strmap.insert(key, QString::fromStdString(xml.getChildData()));
                }
                xml.resetCurrentChild();
            }

            for (const QString &key : intmap.keys()) {
                if (xml.findChild(key.toStdString())) {
                    intmap.insert(key, Util::toInt(xml.getChildData()));
                }
                xml.resetCurrentChild();
            }
        }
    }
    catch (Exception &ex){}
}

void WulforSettings::save(){
    //Do nothing
}

void WulforSettings::parseCmd(const QString &cmd, QString& res) {
    QStringList args = cmd.split(" ", WULFOR_SKIP_EMPTY);

    if (args.size() == 1) {
        res = tr("GUI setting %1: %2").arg(args.at(0)).arg(getStr(args.at(0)));
        return;
    } else if (args.size() > 2)
        return;

    QString sname   = args.at(0);
    QString svalue  = args.at(1);

    setStr(sname, svalue);
    res = tr("Change GUI setting %1 to %2").arg(sname).arg(svalue);
}

void WulforSettings::loadTheme(){
    const QString theme = getStr(WS_APP_THEME);
    if (!theme.isEmpty())
        qApp->setStyle(theme);
    else
        AppTheme::applyPreferredStyle();

    AppTheme::apply();
}

QString WulforSettings::getStr(const QString & key, const QString &default_value) {
    return settings.value(key, default_value).toString();
}

int WulforSettings::getInt(const QString & key, const int &default_value) {
    return settings.value(key, default_value).toInt();
}

bool WulforSettings::getBool(const QString & key, const bool &default_value) {
    return (static_cast<bool>(getInt(key, default_value)));
}

QVariant WulforSettings::getVar(const QString &key, const QVariant &default_value){
    return settings.value(key, default_value);
}

void WulforSettings::setStr(const QString & key, const QString &value) {
    settings.setValue(key, value);

    emit strValueChanged(key, value);
}

void WulforSettings::setInt(const QString & key, int value) {
    settings.setValue(key, value);

    emit intValueChanged(key, value);
}

void WulforSettings::setBool(const QString & key, bool value) {
    setInt(key, static_cast<int>(value));
}

void WulforSettings::setVar(const QString &key, const QVariant &value){
    settings.setValue(key, value);

    emit varValueChanged(key, value);
}
