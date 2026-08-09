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
#include "appicon/AppIcons.h"

#include <QObject>
#include <QPixmap>
#include <QImage>
#include <QMap>
#include <QHash>
#include <QTextCodec>

#include "dcpp/stdinc.h"
#include "dcpp/Singleton.h"
#include "dcpp/ClientManager.h"
#include "dcpp/User.h"
#include "dcpp/CID.h"

#include "WulforSettings.h"

class QAction;
class QHeaderView;
class QAbstractItemView;
class QTreeView;
class QMenu;

#define WICON(x)(WulforUtil::getInstance()->getPixmap((x)))
#define WICON_SIZE(x, s)(WulforUtil::scalePixmap(WulforUtil::getInstance()->getPixmap((x)), (s)))
#define WICON_QICON(x)(WulforUtil::getInstance()->getIcon((x)))

using namespace dcpp;

static const auto _q = [](const std::string &s) { return QString::fromStdString(s); };
static const auto _tq = [](const QString &s) { return s.toStdString(); };

typedef QVariantMap VarMap;

class WulforUtil :
        public QObject,
        public dcpp::Singleton<WulforUtil>
{
    Q_OBJECT

friend class dcpp::Singleton<WulforUtil>;

public:
    /** Theme icon id (catalog lives in AppIcons). */
    typedef AppIcons::Id Icons;

    bool loadUserIcons();

    QIcon getIcon(Icons e);

public Q_SLOTS:
    bool loadIcons();

    QPixmap *getUserIcon(const UserPtr&, bool, bool, const QString&);

    QString getNicks(const CID &cid, const QString& = "");

    QString getAppIconsPath() const;
    QString getEmoticonsPath() const;
    QString getClientIconsPath() const;
    QString getTranslationsPath() const;
    QString getAspellDataPath() const;

    const QPixmap &getPixmapForFile(const QString&);

    void textToHtml(QString&,bool=true);

    QTextCodec *codecForEncoding(const QString&);
    QString qtEnc2DcEnc(QString);
    QString dcEnc2QtEnc(QString);
    QStringList encodings();

    bool getUserCommandParams(const UserCommand& uc, StringMap& params);

    QStringList getLocalIPs();
    QStringList getLocalIfaces();

    QString makeMagnet(const QString&, const int64_t, const QString&);
    static void splitMagnet(const QString &magnet, int64_t &size, QString &tth, QString &name);

    int sortOrderToInt(Qt::SortOrder);
    Qt::SortOrder intToSortOrder(int);

    static QString formatBytes(int64_t bytes);
    static QString formatDisplayBytes(int64_t bytes);

    static void bindActionIcon(QAction *act, Icons icon);

    static qreal iconDeviceRatio() { return AppIcons::deviceRatio(); }
    static QPixmap scalePixmap(const QPixmap &source, int logicalSide,
                               Qt::TransformationMode mode = Qt::SmoothTransformation)
    {
        return AppIcons::scale(source, logicalSide, mode);
    }

    static void headerMenu(QTreeView*);

    /** Restore saved layout; autosize when columns are too narrow once the view is visible. */
    static void restoreTreeHeader(QHeaderView *header, const QByteArray &state);

    /** Re-check column widths after the view becomes visible or its model changes. */
    static void ensureTreeHeaderAutosized(QAbstractItemView *view);

    QString getHubNames(const dcpp::CID&);
    QString getHubNames(const dcpp::UserPtr&);
    QString getHubNames(const QString&);

    QString compactToolTipText(QString, int, QString);

    QMenu *buildUserCmdMenu(const std::string &hub_url, int ctx, QWidget* = nullptr);
    QMenu *buildUserCmdMenu(const StringList& hub_list, int ctx, QWidget* = nullptr);
    QMenu *buildUserCmdMenu(const QList<QString> &hub_list, int ctx, QWidget* = nullptr);

    static bool isTTH(const QString &text);
    static bool revealPath(const QString &path);

    QString getNickViaOnlineUser(const QString &cid, const QString &hintUrl);

Q_SIGNALS:
    void iconsLoaded();

public Q_SLOTS:
    const QPixmap &getPixmap(Icons);
    QString getNicks(const QString&,const QString& = "");
    bool openUrl(const QString&);

private:

    WulforUtil();
    virtual ~WulforUtil();

    bool loadUserIconsFromFile(QString);
    void clearUserIconCache();
    void initFileTypes();

    AppIcons appIcons;

    QString findAppIconsPath() const;
    QString findUserIconsPath() const;
    QString getClientResourcesPath() const;

    QString bin_path;
    QString app_icons_path;

    QPixmap *userIconCache[USERLIST_XPM_COLUMNS][USERLIST_XPM_ROWS];
    QImage  *userIcons;

    QMap<QString, int> connectionSpeeds;
    QMap<QString, Icons> m_FileTypeMap;
    QMap<QString, QString> QtEnc2DCEnc;

    static const QString magnetSignature;

};

Q_DECLARE_METATYPE(WulforUtil*);
Q_DECLARE_METATYPE(WulforUtil::Icons);
