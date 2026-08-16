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

#include "shortcut/ShortcutChord.h"

#include <QHash>
#include <cstdio>

namespace {

struct KeyVocab {
    QHash<int, const char*> map;
    char fkey[35][4];
    char letter[26][2];
    char digit[10][2];

    KeyVocab() {
        for (int i = 0; i < 35; ++i) {
            std::snprintf(fkey[i], sizeof fkey[i], "F%d", i + 1);
            map[Qt::Key_F1 + i] = fkey[i];
        }
        for (int i = 0; i < 26; ++i) {
            letter[i][0] = static_cast<char>('A' + i);
            letter[i][1] = '\0';
            map[Qt::Key_A + i] = letter[i];
        }
        for (int i = 0; i < 10; ++i) {
            digit[i][0] = static_cast<char>('0' + i);
            digit[i][1] = '\0';
            map[Qt::Key_0 + i] = digit[i];
        }
        static const struct { int k; const char *n; } spec[] = {
            { Qt::Key_Escape, "Escape" }, { Qt::Key_Return, "Return" },
            { Qt::Key_Enter, "Enter" }, { Qt::Key_Insert, "Ins" },
            { Qt::Key_Delete, "Delete" }, { Qt::Key_Home, "Home" },
            { Qt::Key_End, "End" }, { Qt::Key_Left, "Left" },
            { Qt::Key_Up, "Up" }, { Qt::Key_Right, "Right" },
            { Qt::Key_Down, "Down" }, { Qt::Key_PageUp, "PgUp" },
            { Qt::Key_PageDown, "PgDown" }, { Qt::Key_CapsLock, "CapsLock" },
            { Qt::Key_NumLock, "NumLock" }, { Qt::Key_ScrollLock, "ScrollLock" },
            { Qt::Key_Exclam, "!" }, { Qt::Key_QuoteDbl, "\"" },
            { Qt::Key_NumberSign, "#" }, { Qt::Key_Dollar, "$" },
            { Qt::Key_Percent, "%" }, { Qt::Key_Ampersand, "&amp;" },
            { Qt::Key_Apostrophe, "\'" }, { Qt::Key_ParenLeft, "(" },
            { Qt::Key_ParenRight, ")" }, { Qt::Key_Asterisk, "*" },
            { Qt::Key_Plus, "+" }, { Qt::Key_Comma, "," },
            { Qt::Key_Minus, "-" }, { Qt::Key_Period, "Period" },
            { Qt::Key_Slash, "/" }, { Qt::Key_Colon, ":" },
            { Qt::Key_Semicolon, ";" }, { Qt::Key_Less, "<" },
            { Qt::Key_Equal, "=" }, { Qt::Key_Greater, ">" },
            { Qt::Key_Question, "?" }, { Qt::Key_At, "@" },
            { Qt::Key_BracketLeft, "[" }, { Qt::Key_Backslash, "\\" },
            { Qt::Key_BracketRight, "]" }, { Qt::Key_Underscore, "_" },
            { Qt::Key_BraceLeft, "{" }, { Qt::Key_Bar, "|" },
            { Qt::Key_BraceRight, "}" }, { Qt::Key_AsciiTilde, "~" },
            { Qt::Key_Space, "Space" }, { Qt::Key_Backspace, "Backspace" },
            { Qt::Key_MediaPlay, "Media Play" }, { Qt::Key_MediaStop, "Media Stop" },
            { Qt::Key_MediaPrevious, "Media Previous" }, { Qt::Key_MediaNext, "Media Next" },
            { Qt::Key_MediaRecord, "Media Record" }, { Qt::Key_MediaLast, "Media Last" },
            { Qt::Key_VolumeUp, "Volume Up" }, { Qt::Key_VolumeDown, "Volume Down" },
            { Qt::Key_VolumeMute, "Volume Mute" }, { Qt::Key_Back, "Back" },
            { Qt::Key_Forward, "Forward" }, { Qt::Key_Stop, "Stop" },
        };
        for (const auto &s : spec)
            map[s.k] = s.n;
    }
};

const KeyVocab &vocab() {
    static const KeyVocab v;
    return v;
}

} // namespace

void ShortcutChord::arm() {
    stop = false;
}

void ShortcutChord::press(int key, Qt::KeyboardModifiers mods, const QString &text) {
    if (stop) {
        keys.clear();
        stop = false;
    }
    const QString name = keyName(key);
    const QStringList extra = modNames(mods);
    if (!name.isEmpty() || !extra.isEmpty()) {
        if (!name.isEmpty() && !keys.contains(name))
            keys << name;
        for (const QString &m : extra) {
            if (!keys.contains(m))
                keys << m;
        }
        return;
    }
    if (!keys.contains(text))
        keys << text;
}

void ShortcutChord::release() {
    stop = true;
}

QString ShortcutChord::text() const {
    QStringList seq;
    if (keys.contains("Shift"))
        seq << "Shift";
    if (keys.contains("Ctrl"))
        seq << "Ctrl";
    if (keys.contains("Alt"))
        seq << "Alt";
    if (keys.contains("Meta"))
        seq << "Meta";
    for (const QString &s : keys) {
        if (s != "Shift" && s != "Ctrl" && s != "Alt" && s != "Meta")
            seq << s;
    }
    return seq.join("+");
}

QString ShortcutChord::keyName(int key) {
    if (key == Qt::Key_Shift || key == Qt::Key_Control || key == Qt::Key_Meta
            || key == Qt::Key_Alt || key == Qt::Key_AltGr)
        return QString();
    const char *name = vocab().map.value(key);
    return name ? QString::fromLatin1(name) : QString();
}

QStringList ShortcutChord::modNames(Qt::KeyboardModifiers mods) {
    QStringList l;
    if (mods & Qt::ShiftModifier)
        l << "Shift";
    if (mods & Qt::ControlModifier)
        l << "Ctrl";
    if (mods & Qt::AltModifier)
        l << "Alt";
    if (mods & Qt::MetaModifier)
        l << "Meta";
    return l;
}
