/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "AppTheme.h"

#include <QApplication>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextFragment>
#include <QVector>

/** Minimum lightness gap matching AppTheme.cpp chat contrast. */
static const int kChatContrast = 50;

/** Preserve source colors so repeated light/dark switches are reversible. */
static const int kSourceColor = QTextFormat::UserProperty + 1;

static bool hasContrast(const QColor &fg, const QColor &bg){
    return fg.isValid() && qAbs(fg.lightness() - bg.lightness()) >= kChatContrast;
}

/** Restore contrast; keep hue for nick/status colors when possible. */
static QColor keepReadable(const QColor &fg){
    const QColor bg = AppTheme::effectiveChatBackground();
    if (hasContrast(fg, bg))
        return fg;

    if (fg.isValid() && fg.hslSaturation() > 15) {
        QColor tinted = fg;
        const int targetL = bg.lightness() < 128 ? 200 : 45;
        tinted.setHsl(fg.hslHue(), qBound(fg.hslSaturation(), 60, 255), targetL);
        if (hasContrast(tinted, bg))
            return tinted;
    }
    return AppTheme::readableChatColor(fg);
}

struct ColorPatch {
    int pos;
    int length;
    QColor color;
    QColor source;
};

static QVector<ColorPatch> colorPatches(QTextDocument *doc){
    QVector<ColorPatch> patches;
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment frag = it.fragment();
            if (!frag.isValid() || frag.length() <= 0)
                continue;

            const QTextCharFormat fmt = frag.charFormat();
            const bool hadFg = fmt.hasProperty(QTextFormat::ForegroundBrush);
            const bool isLink = fmt.isAnchor() || !fmt.anchorHref().isEmpty();
            QColor neu;
            if (isLink)
                neu = AppTheme::linkColor();
            else if (hadFg) {
                const QColor source = fmt.hasProperty(kSourceColor)
                    ? fmt.property(kSourceColor).value<QColor>()
                    : fmt.foreground().color();
                neu = keepReadable(source);
            }
            else
                continue;

            if (!neu.isValid())
                continue;
            const bool storeSource = hadFg && !isLink && !fmt.hasProperty(kSourceColor);
            if (!storeSource && fmt.foreground().color().rgb() == neu.rgb())
                continue;
            patches.append({frag.position(), frag.length(), neu,
                            storeSource ? fmt.foreground().color() : QColor()});
        }
    }
    return patches;
}

static void applyPatch(QTextDocument *doc, const ColorPatch &change){
    QTextCursor cur(doc);
    cur.setPosition(change.pos);
    cur.setPosition(change.pos + change.length, QTextCursor::KeepAnchor);
    QTextCharFormat format;
    if (change.source.isValid())
        format.setProperty(kSourceColor, change.source);
    format.setForeground(change.color);
    cur.mergeCharFormat(format);
}

static void refreshChatDocument(QTextDocument *doc){
    if (!doc || doc->isEmpty())
        return;

    const QVector<ColorPatch> patches = colorPatches(doc);
    QTextCursor edit(doc);
    edit.beginEditBlock();
    for (const ColorPatch &patch : patches)
        applyPatch(doc, patch);
    edit.endEditBlock();
}

void AppTheme::refreshChatViews(){
    const QColor bg = effectiveChatBackground();

    for (QWidget *w : qApp->allWidgets()) {
        if (w->objectName() != QLatin1String("textEdit_CHAT"))
            continue;

        auto *edit = qobject_cast<QTextEdit*>(w);
        if (!edit)
            continue;

        QPalette p = edit->palette();
        p.setColor(QPalette::Base, bg);
        edit->setPalette(p);
        if (QWidget *vp = edit->viewport()) {
            vp->setPalette(p);
            vp->setAutoFillBackground(true);
        }

        const int scroll = edit->verticalScrollBar()->value();
        refreshChatDocument(edit->document());
        edit->verticalScrollBar()->setValue(scroll);
    }
}
