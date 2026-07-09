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

#ifdef USE_ASPELL
#include "SpellCheck.h"
#endif

#include "dcpp/stdinc.h"
#include "dcpp/Util.h"

#include <QApplication>
#include <QFile>
#include <QDir>
#include <QLocale>
#include <QLibraryInfo>
#include <QTranslator>
#include <QtDebug>

using namespace dcpp;

void WulforSettings::loadTranslation(){
    const QString appTranslationFile =
            QDir::fromNativeSeparators(getStr(WS_TRANSLATION_FILE));
    const QString translationsPath =
            QDir::fromNativeSeparators(WulforUtil::getInstance()->getTranslationsPath());

    if (appTranslationFile.isEmpty() || !QFile::exists(appTranslationFile)){
        const QString lcName = QLocale::system().name();

#ifdef _DEBUG_QT_UI
        qDebug() << QString("LANGUAGE=%1").arg(lcName);
#endif
        loadQtTranslation(lcName);
        installTranslator(appTranslator, lcName, "en", translationsPath);

        dcpp::Util::setLang(lcName.toStdString());

        setStr(WS_APP_ASPELL_LANG, lcName);
#ifdef USE_ASPELL
        if (SpellCheck *SC = SpellCheck::getInstance()) {
            SC->setLanguage(lcName);
        }
#endif
    }
    else if (!appTranslationFile.isEmpty() && QFile::exists(appTranslationFile)){
        const QString lcName = (appTranslationFile.split("/").last()).split(".").first();

#ifdef _DEBUG_QT_UI
        qDebug() << QString("LANGUAGE=%1").arg(lcName);
#endif

        loadQtTranslation(lcName);
        installTranslator(appTranslator, lcName, "en", translationsPath);

        dcpp::Util::setLang(lcName.toStdString());

        setStr(WS_APP_ASPELL_LANG, lcName);
#ifdef USE_ASPELL
        if (SpellCheck *SC = SpellCheck::getInstance()) {
            SC->setLanguage(lcName);
        }
#endif
    }
    else {
        setStr(WS_TRANSLATION_FILE, "");
    }
}

void WulforSettings::loadQtTranslation(const QString &lcName){
    if (!WulforUtil::getInstance())
        return;

#if defined (Q_OS_WIN) || defined (Q_OS_MAC)
    const QString translationsPath = WulforUtil::getInstance()->getTranslationsPath();
#else // Other OS
    const QString translationsPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif

    installTranslator(qtTranslator, "qt_" + lcName, "qt_en", translationsPath);
    installTranslator(qtBaseTranslator, "qtbase_" + lcName, "qtbase_en", translationsPath);
}

void WulforSettings::installTranslator(QTranslator &translator,
                                       const QString &defualtName,
                                       const QString &fallbackName,
                                       const QString &path)
{
    if (translator.load(defualtName, path)){
        qApp->installTranslator(&translator);
    }
    else if (translator.load(fallbackName, path)){
        qApp->installTranslator(&translator);
    }
    else {
        qApp->removeTranslator(&translator);
    }
}
