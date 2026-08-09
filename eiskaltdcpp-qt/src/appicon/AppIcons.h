/***************************************************************************
*                                                                         *
*   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QHash>
#include <QIcon>
#include <QPixmap>
#include <QString>

/** Logical user-list cell size; physical pixels come from AppIcons::scale. */
#define USERLIST_ICON_SIZE      16
#define USERLIST_XPM_COLUMNS    9
#define USERLIST_XPM_ROWS       32
#define THEME_ICON_SIZE         22

/**
 * Application theme icon catalog and pixmap cache.
 * Ids stay named ei* for existing call sites (WulforUtil::Icons aliases Id).
 */
class AppIcons
{
public:
    enum Id {
        eiADLS = 0,
        eiAWAY,
        eiBALL_GREEN,
        eiBOOKMARK_ADD,
        eiCHAT,
        eiCLEAR,
        eiCONFIGURE,
        eiCONNECT,
        eiCONNECT_NO,
        eiCONSOLE,
        eiDOWN,
        eiDOWNLIST,
        eiDOWNLOAD,
        eiDOWNLOAD_AS,
        eiEDIT,
        eiEDITADD,
        eiEDITCOPY,
        eiEDITDELETE,
        eiEDITCLEAR,
        eiEMOTICON,
        eiERASER,
        eiEXIT,
        eiFAV,
        eiFAVADD,
        eiFAVREM,
        eiFAVSERVER,
        eiFAVUSERS,
        eiFILECLOSE,
        eiFILEFIND,
        eiFILTER,
        eiFIND,
        eiFOLDER_BLUE,
        eiFREESPACE,
        eiGUI,
        eiGV,
        eiHASHING,
        eiHIDEWINDOW,
        eiHUBMSG,
        eiICON_APPL,
        eiMAGNET,
        eiMESSAGE,
        eiMESSAGE_TRAY_ICON,
        eiOPENLIST,
        eiOPEN_LOG_FILE,
        eiOWN_FILELIST,
        eiPLUGIN,
        eiPMMSG,
        eiRECONNECT,
        eiREFRLIST,
        eiRELOAD,
        eiSERVER,
        eiSPAM,
        eiSPY,
        eiSPLASH,
        eiSTATUS,
        eiTRANSFER,
        eiUP,
        eiUPLIST,
        eiUSERS,
        eiZOOM_IN,
        eiZOOM_OUT,
        eiTOP,
        eiNEXT,
        eiPREVIOUS,
        eiQT_LOGO,
        eiSPEED_LIMIT_ON,
        eiSPEED_LIMIT_OFF,

        eiFILETYPE_APPLICATION,
        eiFILETYPE_ARCHIVE,
        eiFILETYPE_DOCUMENT,
        eiFILETYPE_MP3,
        eiFILETYPE_PICTURE,
        eiFILETYPE_UNKNOWN,
        eiFILETYPE_VIDEO,

        eiCONVERT_EPUB
    };

    typedef QHash<qulonglong, QPixmap> PixmapMap;

    /** Load theme dir (+ optional .rcc). Returns false if any theme file failed. */
    bool load(const QString &themeDir, const QString &resourcePath, bool useSystemTheme);

    const QPixmap &pixmap(Id id);
    QIcon icon(Id id);

    static qreal deviceRatio();
    static QPixmap scale(const QPixmap &source, int logicalSide,
                         Qt::TransformationMode mode = Qt::SmoothTransformation);

private:
    QPixmap loadFile(const QString &file);
    QPixmap fromTheme(const QString &name, bool resource);
    QPixmap fromTheme(const QString &name, bool resource, int side);

    PixmapMap map;
    QString themeDir;
    bool loadError = false;
};
