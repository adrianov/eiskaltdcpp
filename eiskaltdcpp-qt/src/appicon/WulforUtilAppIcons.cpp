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

#include "WulforUtil.h"
#include "WulforSettings.h"

bool WulforUtil::loadIcons()
{
    app_icons_path = findAppIconsPath() + "/";
    const bool ok = appIcons.load(app_icons_path, getClientResourcesPath(),
                                  WBGET("app/use-icon-theme", false));
    if (ok)
        emit iconsLoaded();
    return ok;
}

const QPixmap &WulforUtil::getPixmap(Icons e)
{
    return appIcons.pixmap(e);
}

QIcon WulforUtil::getIcon(Icons e)
{
    return appIcons.icon(e);
}
