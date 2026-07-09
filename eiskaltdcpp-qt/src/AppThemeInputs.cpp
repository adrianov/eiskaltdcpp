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

static QString comboStyle(const QColor &bg, const QColor &border, const QColor &focus){
    return QStringLiteral(
        "QComboBox {"
        "  border: 1px solid %2;"
        "  border-radius: %4px;"
        "  padding: 2px 8px;"
        "  background: %1;"
        "  min-height: 1.2em;"
        "}"
        "QComboBox:focus { border: 1.5px solid %3; }"
        "QComboBox::drop-down { border: none; width: 18px; }"
        "QComboBox::down-arrow { width: 10px; height: 10px; }")
        .arg(bg.name(QColor::HexRgb),
             border.name(QColor::HexRgb),
             focus.name(QColor::HexRgb),
             QString::number(int(AppTheme::controlRadius())));
}

void AppTheme::applyInputPalette(QWidget *widget){
    if (!widget)
        return;

    QPalette p = widget->palette();
    const QColor bg = inputBackground();
    const QColor border = inputBorder(false);
    const QColor focus = inputBorder(true);

    p.setColor(QPalette::Base, bg);
    p.setColor(QPalette::Button, bg);
    widget->setPalette(p);
    widget->setAutoFillBackground(true);

    if (auto *combo = qobject_cast<QComboBox*>(widget)) {
        const QString css = comboStyle(bg, border, focus);
        if (combo->styleSheet() != css)
            combo->setStyleSheet(css);
        return;
    }

    // macOS styles often ignore QTextEdit Base; force the viewport fill.
    if (auto *scroll = qobject_cast<QAbstractScrollArea*>(widget)) {
        QWidget *vp = scroll->viewport();
        vp->setPalette(p);
        vp->setAutoFillBackground(true);
        const QString css = QStringLiteral("background-color: %1; border: none;")
                                .arg(bg.name(QColor::HexRgb));
        if (vp->styleSheet() != css)
            vp->setStyleSheet(css);
    }
}

void AppTheme::applyControlButton(QAbstractButton *button){
    if (!button)
        return;

    button->setProperty("appThemeControl", true);
    button->setCursor(Qt::PointingHandCursor);

    const QColor bg = inputBackground();
    const QColor border = inputBorder(false);
    const QColor hover = isDark() ? bg.lighter(120) : bg.darker(104);
    const QString css = QStringLiteral(
        "QAbstractButton {"
        "  border: 1px solid %2;"
        "  border-radius: %4px;"
        "  background: %1;"
        "  padding: 2px;"
        "}"
        "QAbstractButton:hover { background: %3; }"
        "QAbstractButton:pressed { background: %3; }")
        .arg(bg.name(QColor::HexRgb),
             border.name(QColor::HexRgb),
             hover.name(QColor::HexRgb),
             QString::number(int(controlRadius())));

    if (button->styleSheet() != css)
        button->setStyleSheet(css);
}
