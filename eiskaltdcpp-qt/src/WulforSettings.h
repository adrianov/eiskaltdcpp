/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include "QtCompat.h"

#include <QObject>
#include <QMap>
#include <QTranslator>
#include <QFont>
#include <QSettings>

#include "dcpp/stdinc.h"
#include "dcpp/Singleton.h"

#include "WulforSettingsKeys.h"

class WulforSettings :
        public QObject,
        public dcpp::Singleton<WulforSettings>
{
    Q_OBJECT

    typedef QMap<QString, int> WIntMap;
    typedef QMap<QString, QString> WStrMap;

friend class dcpp::Singleton<WulforSettings>;

public:
    void load();
    void save();

    void loadTranslation();
    void loadTheme();

    void parseCmd(const QString &, QString &);

    bool hasKey(const QString&) const;

public Q_SLOTS:
    QString  getStr(const QString&, const QString &default_value = "") ;
    int      getInt(const QString&, const int &default_value = -1) ;
    bool     getBool(const QString&, const bool &default_value = false);
    QVariant getVar(const QString&, const QVariant &default_value = QVariant());

    void setStr (const QString&, const QString&);
    void setInt (const QString&, int) ;
    void setBool(const QString&, bool);
    void setVar (const QString&, const QVariant &);

Q_SIGNALS:
    void fontChanged(const QString &key, const QString &value);
    void intValueChanged(const QString &key, int value);
    void strValueChanged(const QString &key, const QString &value);
    void varValueChanged(const QString &key, const QVariant &value);

private:
    WulforSettings();
    virtual ~WulforSettings();

    void loadOldConfig(); //load old version of config
    void writeFirstRunDefaults();
    void fillOldConfigMaps();
    void loadQtTranslation(const QString &lcName);
    void installTranslator(QTranslator &translator,
                           const QString &defualtName,
                           const QString &fallbackName,
                           const QString &path);

    QSettings settings;

    QString configFileOld;

    QFont f;

    WIntMap intmap;
    WStrMap strmap;

    QTranslator appTranslator;
    QTranslator qtTranslator;
    QTranslator qtBaseTranslator;
};

static const auto WSGET = [](const QString &key, const QString &default_value = "") -> QString { return WulforSettings::getInstance()->getStr(key, default_value); };
static const auto WSSET = [](const QString &key, const QString &value) { WulforSettings::getInstance()->setStr(key, value); };

static const auto WIGET = [](const QString &key, const int &default_value = -1) -> int { return WulforSettings::getInstance()->getInt(key, default_value); };
static const auto WISET = [](const QString &key, const int &value) { WulforSettings::getInstance()->setInt(key, value); };

static const auto WBGET = [](const QString &key, const bool &default_value = false) -> bool { return WulforSettings::getInstance()->getBool(key, default_value); };
static const auto WBSET = [](const QString &key, const bool &value){ WulforSettings::getInstance()->setBool(key, value); };

static const auto WVGET = [](const QString &key, const QVariant &default_value = QVariant()) -> QVariant { return WulforSettings::getInstance()->getVar(key, default_value); };
static const auto WVSET = [](const QString &key, const QVariant &value){ WulforSettings::getInstance()->setVar(key, value); };

static const auto WSCMD = [](const QString &cmd, QString &res){ WulforSettings::getInstance()->parseCmd(cmd,res); };
