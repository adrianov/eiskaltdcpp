/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "AppTheme.h"
#include "WulforSettings.h"

#include <QColor>

static QColor softWash(QColor c, int alpha)
{
    if (alpha < 0)
        alpha = 0;
    else if (alpha > 255)
        alpha = 255;
    c.setAlpha(alpha);
    return c;
}

QColor AppTheme::sharedFileHighlight()
{
    QColor c;
    const QString stored = WulforSettings::getInstance()->getStr(WS_APP_SHARED_FILES_COLOR);
    if (stored.isEmpty() || isLegacyDefault(stored))
        c = successColor();
    else
        c.setNamedColor(stored);
    if (!c.isValid())
        c = successColor();

    return softWash(c, WulforSettings::getInstance()->getInt(WI_APP_SHARED_FILES_ALPHA, 56));
}

QColor AppTheme::queuedFileHighlight()
{
    QColor c;
    const QString stored = WulforSettings::getInstance()->getStr(WS_APP_QUEUED_FILES_COLOR);
    if (stored.isEmpty() || isLegacyDefault(stored))
        c = linkColor();
    else
        c.setNamedColor(stored);
    if (!c.isValid())
        c = linkColor();

    return softWash(c, WulforSettings::getInstance()->getInt(WI_APP_QUEUED_FILES_ALPHA, 56));
}
