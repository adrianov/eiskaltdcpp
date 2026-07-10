/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "AppTheme.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QApplication>
#include <QComboBox>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QWidget>

QColor AppTheme::inputBackground(){
    const QPalette &pal = qApp->palette();
    QColor bg = pal.color(QPalette::Base);
    const QColor window = pal.color(QPalette::Window);
    if (!isDark()) {
        // Near-white Base reads as blank space; slight fill like SwiftUI rounded fields.
        if (bg.lightness() > 230)
            return QColor(0xF2, 0xF2, 0xF7);
        return bg;
    }
    if (qAbs(bg.lightness() - window.lightness()) >= 12)
        return bg;

    const QColor control = pal.color(QPalette::Button);
    if (qAbs(control.lightness() - window.lightness()) >= 12)
        return control;

    return bg.lighter(118);
}

QColor AppTheme::inputBorder(bool focused){
    const QPalette &pal = qApp->palette();

    if (focused)
        return pal.color(QPalette::Highlight);
    if (!isDark())
        return QColor(0xC6, 0xC6, 0xC8);
    return QColor(255, 255, 255, 46);
}

void AppTheme::paintControlBorder(QPainter *painter, const QRectF &rect, bool focused){
    if (!painter)
        return;

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(inputBorder(focused), focused ? 1.5 : 1.0));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(rect.adjusted(0.5, 0.5, -0.5, -0.5), controlRadius(), controlRadius());
}

QColor AppTheme::readableColor(const QColor &bg, const QColor &preferred){
    if (preferred.isValid() && qAbs(preferred.lightness() - bg.lightness()) >= 90)
        return preferred;
    return bg.lightness() < 128 ? QColor(Qt::white) : QColor(Qt::black);
}

AppTheme::InputColors AppTheme::inputColors(){
    const QPalette &pal = qApp->palette();
    InputColors c;
    c.bg = inputBackground();
    c.text = readableColor(c.bg, pal.color(QPalette::Text));
    c.muted = readableColor(c.bg, c.text.darker(150));
    c.selBg = pal.color(QPalette::Highlight);
    c.selFg = readableColor(c.selBg, pal.color(QPalette::HighlightedText));
    return c;
}

static void setInputColors(QPalette &p, const AppTheme::InputColors &c){
    for (const auto group : {QPalette::Active, QPalette::Inactive}) {
        p.setColor(group, QPalette::Base, c.bg);
        p.setColor(group, QPalette::Button, c.bg);
        p.setColor(group, QPalette::Text, c.text);
        p.setColor(group, QPalette::WindowText, c.text);
        p.setColor(group, QPalette::ButtonText, c.text);
        p.setColor(group, QPalette::Highlight, c.selBg);
        p.setColor(group, QPalette::HighlightedText, c.selFg);
    }
    p.setColor(QPalette::Disabled, QPalette::Text, c.muted);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, c.muted);
}

void AppTheme::applyInputPalette(QWidget *widget){
    if (!widget)
        return;

    const InputColors c = inputColors();

    QPalette p = widget->palette();
    setInputColors(p, c);
    widget->setPalette(p);
    widget->setAutoFillBackground(true);

    // Combo chrome comes from the app-wide sheet; only item/view colors here.
    if (auto *combo = qobject_cast<QComboBox*>(widget)) {
        for (int i = 0; i < combo->count(); ++i)
            combo->setItemData(i, c.text, Qt::ForegroundRole);
        if (QAbstractItemView *view = combo->view()) {
            QPalette vp = view->palette();
            setInputColors(vp, c);
            view->setPalette(vp);
        }
        return;
    }

    // macOS styles often ignore QTextEdit Base; force the viewport fill.
    if (auto *scroll = qobject_cast<QAbstractScrollArea*>(widget)) {
        QWidget *vp = scroll->viewport();
        vp->setPalette(p);
        vp->setAutoFillBackground(true);
        const QString css = QStringLiteral("background-color: %1; color: %2; border: none;")
                                .arg(c.bg.name(QColor::HexRgb), c.text.name(QColor::HexRgb));
        if (vp->styleSheet() != css)
            vp->setStyleSheet(css);
    }
}

void AppTheme::applyControlButton(QAbstractButton *button){
    if (!button)
        return;

    button->setProperty("appThemeControl", true);
    button->setCursor(Qt::PointingHandCursor);

    const InputColors c = inputColors();
    const QColor hover = isDark() ? c.bg.lighter(120) : c.bg.darker(104);

    const QString css = QStringLiteral(
        "QAbstractButton {"
        "  border: 1px solid %2;"
        "  border-radius: %5px;"
        "  background: %1;"
        "  color: %3;"
        "  padding: 2px;"
        "}"
        "QAbstractButton:hover, QAbstractButton:pressed { background: %4; }")
        .arg(c.bg.name(QColor::HexRgb),
             inputBorder(false).name(QColor::HexRgb),
             c.text.name(QColor::HexRgb),
             hover.name(QColor::HexRgb),
             QString::number(int(controlRadius())));

    if (button->styleSheet() != css)
        button->setStyleSheet(css);
}
