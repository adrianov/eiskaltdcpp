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

#include <QString>
#include <QStringList>
#include <Qt>

/** In-progress key combination captured by ShortcutGetter. */
class ShortcutChord {
public:
    void arm();
    void press(int key, Qt::KeyboardModifiers mods, const QString &text);
    void release();
    QString text() const;

private:
    static QString keyName(int key);
    static QStringList modNames(Qt::KeyboardModifiers mods);

    QStringList keys;
    bool stop = false;
};
