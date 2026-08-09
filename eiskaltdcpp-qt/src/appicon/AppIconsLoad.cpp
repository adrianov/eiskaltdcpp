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

#include "appicon/AppIcons.h"

#include <QDir>
#include <QFile>
#include <QResource>

#include "icons/gv.xpm"

namespace {
const int PXMTHEMESIDE = THEME_ICON_SIZE;
}

bool AppIcons::load(const QString &themeDirPath, const QString &resourcePath, bool useSystemTheme)
{
    loadError = false;
    themeDir = themeDirPath;
    if (!themeDir.endsWith(QLatin1Char('/')))
        themeDir += QLatin1Char('/');

    bool resourceFound = false;
    if (QFile::exists(resourcePath) && !useSystemTheme)
        resourceFound = QResource::registerResource(resourcePath);

    map.clear();

    map[eiAWAY]         = fromTheme("im-user-away", resourceFound);
    map[eiBOOKMARK_ADD] = fromTheme("bookmark-new", resourceFound);
    map[eiCLEAR]        = fromTheme("edit-clear",   resourceFound);
    map[eiCONFIGURE]    = fromTheme("configure",    resourceFound);
    map[eiCONVERT_EPUB] = fromTheme("convert-to-epub", resourceFound);
    map[eiCONNECT]      = fromTheme("network-connect", resourceFound);
    map[eiCONNECT_NO]   = fromTheme("network-disconnect", resourceFound);
    map[eiDOWN]         = fromTheme("go-down", resourceFound);
    map[eiDOWNLIST]     = fromTheme("go-down-search", resourceFound);
    map[eiDOWNLOAD]     = fromTheme("download", resourceFound);
    map[eiDOWNLOAD_AS]  = fromTheme("download", resourceFound);
    map[eiEDIT]         = fromTheme("document-edit", resourceFound);
    map[eiEDITADD]      = fromTheme("list-add", resourceFound);
    map[eiEDITCOPY]     = fromTheme("edit-copy", resourceFound);
    map[eiEDITDELETE]   = fromTheme("edit-delete", resourceFound);
    map[eiEDITCLEAR]    = fromTheme("edit-clear-locationbar-rtl", resourceFound, 16);
    map[eiEMOTICON]     = fromTheme("face-smile", resourceFound);
    map[eiEXIT]         = fromTheme("application-exit", resourceFound);
    map[eiFILECLOSE]    = fromTheme("dialog-close", resourceFound);
    map[eiFILEFIND]     = fromTheme("edit-find", resourceFound);
    map[eiFILTER]       = fromTheme("view-filter", resourceFound);
    map[eiFOLDER_BLUE]  = fromTheme("folder-blue", resourceFound);
    map[eiHIDEWINDOW]   = fromTheme("view-close", resourceFound);
    map[eiUP]           = fromTheme("go-up", resourceFound);
    map[eiUPLIST]       = fromTheme("go-up-search", resourceFound);
    map[eiZOOM_IN]      = fromTheme("zoom-in", resourceFound);
    map[eiZOOM_OUT]     = fromTheme("zoom-out", resourceFound);
    map[eiTOP]          = fromTheme("go-top", resourceFound);
    map[eiNEXT]         = fromTheme("go-next", resourceFound);
    map[eiPREVIOUS]     = fromTheme("go-previous", resourceFound);

    map[eiFILETYPE_APPLICATION] = fromTheme("application-x-executable", resourceFound);
    map[eiFILETYPE_ARCHIVE]     = fromTheme("application-x-archive", resourceFound);
    map[eiFILETYPE_DOCUMENT]    = fromTheme("text-x-generic", resourceFound);
    map[eiFILETYPE_MP3]         = fromTheme("audio-x-generic", resourceFound);
    map[eiFILETYPE_PICTURE]     = fromTheme("image-x-generic", resourceFound);
    map[eiFILETYPE_UNKNOWN]     = fromTheme("unknown", resourceFound);
    map[eiFILETYPE_VIDEO]       = fromTheme("video-x-generic", resourceFound);

    map[eiADLS]         = fromTheme("adls", resourceFound);
    map[eiBALL_GREEN]   = fromTheme("ball_green", resourceFound);
    map[eiCHAT]         = fromTheme("chat", resourceFound);
    map[eiCONSOLE]      = fromTheme("console", resourceFound);
    map[eiERASER]       = fromTheme("eraser", resourceFound);
    map[eiFAV]          = fromTheme("fav", resourceFound);
    map[eiFAVADD]       = fromTheme("favadd", resourceFound);
    map[eiFAVREM]       = fromTheme("favrem", resourceFound);
    map[eiFAVSERVER]    = fromTheme("favserver", resourceFound);
    map[eiFAVUSERS]     = fromTheme("favusers", resourceFound);
    map[eiFIND]         = fromTheme("find", resourceFound);
    map[eiFREESPACE]    = fromTheme("freespace", resourceFound);
    map[eiGUI]          = fromTheme("gui", resourceFound);
    map[eiGV]           = scale(QPixmap(gv_xpm), PXMTHEMESIDE);
    map[eiHASHING]      = fromTheme("hashing", resourceFound);
    map[eiHUBMSG]       = fromTheme("hubmsg", resourceFound);
    map[eiICON_APPL]    = fromTheme("icon_appl_big", resourceFound, 128);
    map[eiMAGNET]       = fromTheme("magnet", resourceFound);
    map[eiMESSAGE]      = fromTheme("message", resourceFound);
    map[eiMESSAGE_TRAY_ICON] = fromTheme("icon_msg_big", resourceFound, 128);
    map[eiOWN_FILELIST] = fromTheme("own_filelist", resourceFound);
    map[eiOPENLIST]     = fromTheme("openlist", resourceFound);
    map[eiOPEN_LOG_FILE]= fromTheme("log_file", resourceFound);
    map[eiPLUGIN]       = fromTheme("plugin", resourceFound);
    map[eiPMMSG]        = fromTheme("pmmsg", resourceFound);
    map[eiRECONNECT]    = fromTheme("reconnect", resourceFound);
    map[eiREFRLIST]     = fromTheme("refrlist", resourceFound);
    map[eiRELOAD]       = fromTheme("reload", resourceFound);
    map[eiSERVER]       = fromTheme("server", resourceFound);
    map[eiSPAM]         = fromTheme("spam", resourceFound);
    map[eiSPY]          = fromTheme("spy", resourceFound);
    map[eiSPEED_LIMIT_OFF]  = fromTheme("slow_off", resourceFound);
    map[eiSPEED_LIMIT_ON]   = fromTheme("slow", resourceFound);

    map[eiSPLASH]       = QPixmap();
    map[eiSTATUS]       = fromTheme("status", resourceFound);
    map[eiTRANSFER]     = fromTheme("transfer", resourceFound);
    map[eiUSERS]        = fromTheme("users", resourceFound);
    map[eiQT_LOGO]      = fromTheme("qt-logo", resourceFound);

    return !loadError;
}

QPixmap AppIcons::loadFile(const QString &file)
{
    QPixmap p;
    const QString f = QDir::toNativeSeparators(themeDir + file);
    if (p.load(f))
        return p;

    printf("loadPixmap: Can't load '%s'\n", f.toUtf8().constData());
    loadError = true;
    return scale(QPixmap(gv_xpm), PXMTHEMESIDE);
}

QPixmap AppIcons::fromTheme(const QString &name, bool resource)
{
    return fromTheme(name, resource, PXMTHEMESIDE);
}

QPixmap AppIcons::fromTheme(const QString &name, bool resource, int side)
{
    const QPixmap source = resource ? QPixmap(":/" + name + ".png") : loadFile(name + ".png");
    return scale(source, side);
}

const QPixmap &AppIcons::pixmap(Id id)
{
    return map[static_cast<qulonglong>(id)];
}

QIcon AppIcons::icon(Id id)
{
    return QIcon(pixmap(id));
}
