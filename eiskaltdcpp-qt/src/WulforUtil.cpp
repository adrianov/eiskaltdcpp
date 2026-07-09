/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "MainWindow.h"
#include "Magnet.h"
#include "WulforUtil.h"
#include "ArenaWidgetFactory.h"
#include "icons/gv.xpm"

#ifdef HAVE_IFADDRS_H
#include <sys/types.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <fcntl.h>
#endif
#include "TransferDisplay.h"

#include "dcpp/ClientManager.h"
#include "dcpp/SettingsManager.h"
#include "dcpp/Util.h"
#include "dcpp/AdcHub.h"

#include <QDir>
#include <QFileInfo>
#include <QApplication>
#if QT_VERSION >= 0x050000
#include <QGuiApplication>
#include <QScreen>
#endif
#include <QInputDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QAbstractItemModel>
#include <QHostAddress>
#include <QMenu>
#include <QAction>
#include <QTreeView>
#include <QIcon>
#include <QResource>
#include <memory>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QPushButton>
#include <QFrame>
#include <QHBoxLayout>
#include <QRegExp>
#include <QProcess>

#if QT_VERSION >= 0x050000
#include <QUrlQuery>
#endif

#include "SearchFrame.h"
#include "extra/magnet.h"

#ifndef CLIENT_DATA_DIR
#define CLIENT_DATA_DIR ""
#endif

#ifndef CLIENT_ICONS_DIR
#define CLIENT_ICONS_DIR ""
#endif

#ifndef CLIENT_TRANSLATIONS_DIR
#define CLIENT_TRANSLATIONS_DIR ""
#endif

#ifndef CLIENT_SOUNDS_DIR
#define CLIENT_SOUNDS_DIR ""
#endif

#ifndef CLIENT_RES_DIR
#define CLIENT_RES_DIR ""
#endif

using namespace dcpp;

const QString WulforUtil::magnetSignature = "magnet:?xt=urn:tree:tiger:";

WulforUtil::WulforUtil()
{
    qRegisterMetaType< QMap<QString,QVariant> >("VarMap");
    qRegisterMetaType< QList<QVariantMap> >("QList<QVariantMap>");
    qRegisterMetaType<dcpp::UserPtr>("dcpp::UserPtr");
    qRegisterMetaType< QMap<QString,QString> >("QMap<QString,QString>");
    qRegisterMetaType< dcpp::Identity >("dcpp::Identity");

    memset(userIconCache, 0, sizeof (userIconCache));

    userIcons = new QImage();

    connectionSpeeds["0.005"]   = 0;
    connectionSpeeds["0.01"]    = 0;
    connectionSpeeds["0.02"]    = 0;
    connectionSpeeds["0.05"]    = 0;
    connectionSpeeds["0.1"]     = 1;
    connectionSpeeds["0.2"]     = 1;
    connectionSpeeds["0.5"]     = 2;
    connectionSpeeds["1"]       = 3;
    connectionSpeeds["2"]       = 4;
    connectionSpeeds["5"]       = 5;
    connectionSpeeds["10"]      = 5;
    connectionSpeeds["20"]      = 5;
    connectionSpeeds["50"]      = 5;
    connectionSpeeds["100"]     = 5;
    connectionSpeeds["1000"]    = 6;

    QtEnc2DCEnc["CP949"]        = "CP949 (Korean)";
    QtEnc2DCEnc["GB18030-0"]    = "GB18030 (Chinese)";
    QtEnc2DCEnc["ISO 8859-2"]   = "ISO-8859-2 (Central Europe)";
    QtEnc2DCEnc["ISO 8859-7"]   = "ISO-8859-7 (Greek)";
    QtEnc2DCEnc["ISO 8859-8"]   = "ISO-8859-8 (Hebrew)";
    QtEnc2DCEnc["ISO 8859-9"]   = "ISO-8859-9 (Turkish)";
    QtEnc2DCEnc["ISO 2022-JP"]  = "ISO-2022-JP (Japanese)";
    QtEnc2DCEnc["KOI8-R"]       = "KOI8-R (Cyrillic)";
    QtEnc2DCEnc["UTF-8"]        = "UTF-8 (Unicode)";
    QtEnc2DCEnc["TIS-620"]      = "TIS-620 (Thai)";
    QtEnc2DCEnc["WINDOWS-1250"] = "CP1250 (Central Europe)";
    QtEnc2DCEnc["WINDOWS-1251"] = "CP1251 (Cyrillic)";
    QtEnc2DCEnc["WINDOWS-1252"] = "CP1252 (Western Europe)";
    QtEnc2DCEnc["WINDOWS-1256"] = "CP1256 (Arabic)";
    QtEnc2DCEnc["WINDOWS-1257"] = "CP1257 (Baltic)";

    bin_path = qApp->applicationDirPath() + "/";
    app_icons_path = findAppIconsPath() + "/";

    initFileTypes();
}

WulforUtil::~WulforUtil(){
    delete userIcons;

    clearUserIconCache();
}

bool WulforUtil::loadUserIcons(){
    return loadUserIconsFromFile(findUserIconsPath() + QString("/usericons.png"));
}

