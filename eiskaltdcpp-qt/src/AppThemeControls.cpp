/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "AppTheme.h"
#include "ChatEdit.h"
#include "LineEdit.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QPalette>
#include <QProgressBar>
#include <QStyleFactory>
#include <QWidget>

bool AppTheme::isDark(const QPalette &palette){
    const QColor base = palette.color(QPalette::Base);
    if (base.isValid() && qAbs(base.lightness() - palette.color(QPalette::Window).lightness()) >= 8)
        return base.lightness() < 128;

    return palette.color(QPalette::Window).lightness() < 128;
}

bool AppTheme::isDark(){
    return isDark(qApp->palette());
}

qreal AppTheme::controlRadius(){
    return 6.0;
}

void AppTheme::applyPreferredStyle(){
    // Empty WS_APP_THEME ("Default") → Fusion: flat, consistent, SwiftUI-adjacent chrome.
    if (!QStyleFactory::keys().contains(QStringLiteral("Fusion"), Qt::CaseInsensitive))
        return;

    qApp->setStyle(QStringLiteral("Fusion"));
}

void AppTheme::applyProgressBar(QProgressBar *bar){
    if (!bar)
        return;

    const QPalette palette = qApp->palette();
    const QString css = QStringLiteral(
        "QProgressBar {"
        "  border: 1px solid %1;"
        "  border-radius: 4px;"
        "  background-color: %2;"
        "  color: %3;"
        "  text-align: center;"
        "  padding: 0px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: %4;"
        "  border-radius: 3px;"
        "}")
        .arg(palette.color(QPalette::Mid).name(),
             palette.color(QPalette::Base).name(),
             palette.color(QPalette::Text).name(),
             linkColor().name());

    if (bar->styleSheet() != css)
        bar->setStyleSheet(css);
}

void AppTheme::apply(){
    // App-wide sheet reaches combos created after this sweep (hub/search frames).
    const QString comboCss = comboStyleSheet();
    if (qApp->styleSheet() != comboCss)
        qApp->setStyleSheet(comboCss);

    const auto widgets = qApp->allWidgets();
    for (QWidget *w : widgets) {
        if (qobject_cast<ChatEdit*>(w) || qobject_cast<LineEdit*>(w) || qobject_cast<QComboBox*>(w))
            applyInputPalette(w);
        else if (auto *bar = qobject_cast<QProgressBar*>(w))
            applyProgressBar(bar);
        else if (auto *view = qobject_cast<QAbstractItemView*>(w))
            view->viewport()->update();
        else if (auto *btn = qobject_cast<QAbstractButton*>(w)) {
            if (btn->property("appThemeControl").toBool())
                applyControlButton(btn);
        }
    }
}
