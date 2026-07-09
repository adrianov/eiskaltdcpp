/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "WulforUtil.h"
#include "WulforSettings.h"

#include <QFile>
#include <QIcon>
#include <QPixmap>
#include <QDir>
#include <QResource>

#include "icons/gv.xpm"

namespace {
const int PXMTHEMESIDE = THEME_ICON_SIZE;
}

bool WulforUtil::loadIcons(){
    m_bError = false;

    app_icons_path = findAppIconsPath() + "/";

    const QString fname = getClientResourcesPath();
    bool resourceFound = false;
    if (QFile(fname).exists() && !WBGET("app/use-icon-theme", false))
        resourceFound = QResource::registerResource(fname);

    m_PixmapMap.clear();

    m_PixmapMap[eiAWAY]         = FROMTHEME("im-user-away", resourceFound);
    m_PixmapMap[eiBOOKMARK_ADD] = FROMTHEME("bookmark-new", resourceFound);
    m_PixmapMap[eiCLEAR]        = FROMTHEME("edit-clear",   resourceFound);
    m_PixmapMap[eiCONFIGURE]    = FROMTHEME("configure",    resourceFound);
    m_PixmapMap[eiCONNECT]      = FROMTHEME("network-connect", resourceFound);
    m_PixmapMap[eiCONNECT_NO]   = FROMTHEME("network-disconnect", resourceFound);
    m_PixmapMap[eiDOWN]         = FROMTHEME("go-down", resourceFound);
    m_PixmapMap[eiDOWNLIST]     = FROMTHEME("go-down-search", resourceFound);
    m_PixmapMap[eiDOWNLOAD]     = FROMTHEME("download", resourceFound);
    m_PixmapMap[eiDOWNLOAD_AS]  = FROMTHEME("download", resourceFound);
    m_PixmapMap[eiEDIT]         = FROMTHEME("document-edit", resourceFound);
    m_PixmapMap[eiEDITADD]      = FROMTHEME("list-add", resourceFound);
    m_PixmapMap[eiEDITCOPY]     = FROMTHEME("edit-copy", resourceFound);
    m_PixmapMap[eiEDITDELETE]   = FROMTHEME("edit-delete", resourceFound);
    m_PixmapMap[eiEDITCLEAR]    = FROMTHEME_SIDE("edit-clear-locationbar-rtl", resourceFound, 16);
    m_PixmapMap[eiEMOTICON]     = FROMTHEME("face-smile", resourceFound);
    m_PixmapMap[eiEXIT]         = FROMTHEME("application-exit", resourceFound);
    m_PixmapMap[eiFILECLOSE]    = FROMTHEME("dialog-close", resourceFound);
    m_PixmapMap[eiFILEFIND]     = FROMTHEME("edit-find", resourceFound);
    m_PixmapMap[eiFILTER]       = FROMTHEME("view-filter", resourceFound);
    m_PixmapMap[eiFOLDER_BLUE]  = FROMTHEME("folder-blue", resourceFound);
    m_PixmapMap[eiHIDEWINDOW]   = FROMTHEME("view-close", resourceFound);
    m_PixmapMap[eiUP]           = FROMTHEME("go-up", resourceFound);
    m_PixmapMap[eiUPLIST]       = FROMTHEME("go-up-search", resourceFound);
    m_PixmapMap[eiZOOM_IN]      = FROMTHEME("zoom-in", resourceFound);
    m_PixmapMap[eiZOOM_OUT]     = FROMTHEME("zoom-out", resourceFound);
    m_PixmapMap[eiTOP]          = FROMTHEME("go-top", resourceFound);
    m_PixmapMap[eiNEXT]         = FROMTHEME("go-next", resourceFound);
    m_PixmapMap[eiPREVIOUS]     = FROMTHEME("go-previous", resourceFound);

    m_PixmapMap[eiFILETYPE_APPLICATION] = FROMTHEME("application-x-executable", resourceFound);
    m_PixmapMap[eiFILETYPE_ARCHIVE]     = FROMTHEME("application-x-archive", resourceFound);
    m_PixmapMap[eiFILETYPE_DOCUMENT]    = FROMTHEME("text-x-generic", resourceFound);
    m_PixmapMap[eiFILETYPE_MP3]         = FROMTHEME("audio-x-generic", resourceFound);
    m_PixmapMap[eiFILETYPE_PICTURE]     = FROMTHEME("image-x-generic", resourceFound);
    m_PixmapMap[eiFILETYPE_UNKNOWN]     = FROMTHEME("unknown", resourceFound);
    m_PixmapMap[eiFILETYPE_VIDEO]       = FROMTHEME("video-x-generic", resourceFound);

    m_PixmapMap[eiADLS]         = FROMTHEME("adls", resourceFound);
    m_PixmapMap[eiBALL_GREEN]   = FROMTHEME("ball_green", resourceFound);
    m_PixmapMap[eiCHAT]         = FROMTHEME("chat", resourceFound);
    m_PixmapMap[eiCONSOLE]      = FROMTHEME("console", resourceFound);
    m_PixmapMap[eiERASER]       = FROMTHEME("eraser", resourceFound);
    m_PixmapMap[eiFAV]          = FROMTHEME("fav", resourceFound);
    m_PixmapMap[eiFAVADD]       = FROMTHEME("favadd", resourceFound);
    m_PixmapMap[eiFAVREM]       = FROMTHEME("favrem", resourceFound);
    m_PixmapMap[eiFAVSERVER]    = FROMTHEME("favserver", resourceFound);
    m_PixmapMap[eiFAVUSERS]     = FROMTHEME("favusers", resourceFound);
    m_PixmapMap[eiFIND]         = FROMTHEME("find", resourceFound);
    m_PixmapMap[eiFREESPACE]    = FROMTHEME("freespace", resourceFound);
    m_PixmapMap[eiGUI]          = FROMTHEME("gui", resourceFound);
    m_PixmapMap[eiGV]           = scalePixmap(QPixmap(gv_xpm), PXMTHEMESIDE);
    m_PixmapMap[eiHASHING]      = FROMTHEME("hashing", resourceFound);
    m_PixmapMap[eiHUBMSG]       = FROMTHEME("hubmsg", resourceFound);
    m_PixmapMap[eiICON_APPL]    = FROMTHEME_SIDE("icon_appl_big", resourceFound, 128);
    m_PixmapMap[eiMAGNET]       = FROMTHEME("magnet", resourceFound);
    m_PixmapMap[eiMESSAGE]      = FROMTHEME("message", resourceFound);
    m_PixmapMap[eiMESSAGE_TRAY_ICON] = FROMTHEME_SIDE("icon_msg_big", resourceFound, 128);
    m_PixmapMap[eiOWN_FILELIST] = FROMTHEME("own_filelist", resourceFound);
    m_PixmapMap[eiOPENLIST]     = FROMTHEME("openlist", resourceFound);
    m_PixmapMap[eiOPEN_LOG_FILE]= FROMTHEME("log_file", resourceFound);
    m_PixmapMap[eiPLUGIN]       = FROMTHEME("plugin", resourceFound);
    m_PixmapMap[eiPMMSG]        = FROMTHEME("pmmsg", resourceFound);
    m_PixmapMap[eiRECONNECT]    = FROMTHEME("reconnect", resourceFound);
    m_PixmapMap[eiREFRLIST]     = FROMTHEME("refrlist", resourceFound);
    m_PixmapMap[eiRELOAD]       = FROMTHEME("reload", resourceFound);
    m_PixmapMap[eiSERVER]       = FROMTHEME("server", resourceFound);
    m_PixmapMap[eiSPAM]         = FROMTHEME("spam", resourceFound);
    m_PixmapMap[eiSPY]          = FROMTHEME("spy", resourceFound);
    m_PixmapMap[eiSPEED_LIMIT_OFF]  = FROMTHEME("slow_off", resourceFound);
    m_PixmapMap[eiSPEED_LIMIT_ON]   = FROMTHEME("slow", resourceFound);

    m_PixmapMap[eiSPLASH]       = QPixmap();
    m_PixmapMap[eiSTATUS]       = FROMTHEME("status", resourceFound);
    m_PixmapMap[eiTRANSFER]     = FROMTHEME("transfer", resourceFound);
    m_PixmapMap[eiUSERS]        = FROMTHEME("users", resourceFound);
    m_PixmapMap[eiQT_LOGO]      = FROMTHEME("qt-logo", resourceFound);

    if (!m_bError)
        emit iconsLoaded();

    return !m_bError;
}

QPixmap WulforUtil::loadPixmap(const QString &file){
    QString f;
    QPixmap p;

    f = app_icons_path + file;
    f = QDir::toNativeSeparators(f);

    if (p.load(f))
        return p;

    printf("loadPixmap: Can't load '%s'\n", f.toUtf8().constData());

    m_bError = true;

    p = scalePixmap(QPixmap(gv_xpm), PXMTHEMESIDE);

    return p;
}

const QPixmap &WulforUtil::getPixmap(enum WulforUtil::Icons e){
    return m_PixmapMap[static_cast<qulonglong>(e)];
}

QIcon WulforUtil::getIcon(Icons e)
{
    return QIcon(getPixmap(e));
}

