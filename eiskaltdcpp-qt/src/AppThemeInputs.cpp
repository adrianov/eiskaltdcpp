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

static QColor readableOn(const QColor &bg, const QColor &preferred){
    if (preferred.isValid() && qAbs(preferred.lightness() - bg.lightness()) >= 90)
        return preferred;
    return bg.lightness() < 128 ? QColor(Qt::white) : QColor(Qt::black);
}

// Named tokens — never QString::arg with data that may contain "%23" / "%3C".
static QString comboStyle(const QColor &bg, const QColor &text, const QColor &border,
                          const QColor &focus, const QColor &selBg, const QColor &selFg){
    QString css = QStringLiteral(
        "QComboBox {"
        "  border: 1px solid @border@;"
        "  border-radius: @radius@px;"
        "  padding: 2px 8px;"
        "  background: @bg@;"
        "  color: @text@;"
        "  min-height: 1.2em;"
        "}"
        "QComboBox:!editable, QComboBox:!editable:on, QComboBox:disabled {"
        "  color: @text@;"
        "}"
        "QComboBox:focus { border: 1.5px solid @focus@; }"
        "QComboBox::drop-down { border: none; width: 18px; }"
        "QComboBox QAbstractItemView {"
        "  background: @bg@;"
        "  color: @text@;"
        "  selection-background-color: @selBg@;"
        "  selection-color: @selFg@;"
        "  outline: 0;"
        "}");
    css.replace(QLatin1String("@bg@"), bg.name(QColor::HexRgb));
    css.replace(QLatin1String("@text@"), text.name(QColor::HexRgb));
    css.replace(QLatin1String("@border@"), border.name(QColor::HexRgb));
    css.replace(QLatin1String("@focus@"), focus.name(QColor::HexRgb));
    css.replace(QLatin1String("@radius@"), QString::number(int(AppTheme::controlRadius())));
    css.replace(QLatin1String("@selBg@"), selBg.name(QColor::HexRgb));
    css.replace(QLatin1String("@selFg@"), selFg.name(QColor::HexRgb));
    return css;
}

static void setInputColors(QPalette &p, const QColor &bg, const QColor &text,
                           const QColor &selBg, const QColor &selFg){
    for (const auto group : {QPalette::Active, QPalette::Inactive}) {
        p.setColor(group, QPalette::Base, bg);
        p.setColor(group, QPalette::Button, bg);
        p.setColor(group, QPalette::Text, text);
        p.setColor(group, QPalette::WindowText, text);
        p.setColor(group, QPalette::ButtonText, text);
        p.setColor(group, QPalette::Highlight, selBg);
        p.setColor(group, QPalette::HighlightedText, selFg);
    }
    const QColor muted = readableOn(bg, text.darker(150));
    p.setColor(QPalette::Disabled, QPalette::Text, muted);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, muted);
}

static void setComboItemFg(QComboBox *combo, const QColor &text){
    for (int i = 0; i < combo->count(); ++i)
        combo->setItemData(i, text, Qt::ForegroundRole);
}

void AppTheme::applyInputPalette(QWidget *widget){
    if (!widget)
        return;

    const QPalette &appPal = qApp->palette();
    const QColor bg = inputBackground();
    const QColor text = readableOn(bg, appPal.color(QPalette::Text));
    const QColor border = inputBorder(false);
    const QColor focus = inputBorder(true);
    const QColor selBg = appPal.color(QPalette::Highlight);
    const QColor selFg = readableOn(selBg, appPal.color(QPalette::HighlightedText));

    QPalette p = widget->palette();
    setInputColors(p, bg, text, selBg, selFg);
    widget->setPalette(p);
    widget->setAutoFillBackground(true);

    if (auto *combo = qobject_cast<QComboBox*>(widget)) {
        const QString css = comboStyle(bg, text, border, focus, selBg, selFg);
        if (combo->styleSheet() != css)
            combo->setStyleSheet(css);
        setComboItemFg(combo, text);
        if (QAbstractItemView *view = combo->view()) {
            QPalette vp = view->palette();
            setInputColors(vp, bg, text, selBg, selFg);
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
                                .arg(bg.name(QColor::HexRgb), text.name(QColor::HexRgb));
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
    const QColor text = readableOn(bg, qApp->palette().color(QPalette::ButtonText));
    const QColor border = inputBorder(false);
    const QColor hover = isDark() ? bg.lighter(120) : bg.darker(104);

    QString css = QStringLiteral(
        "QAbstractButton {"
        "  border: 1px solid @border@;"
        "  border-radius: @radius@px;"
        "  background: @bg@;"
        "  color: @text@;"
        "  padding: 2px;"
        "}"
        "QAbstractButton:hover, QAbstractButton:pressed { background: @hover@; }");
    css.replace(QLatin1String("@bg@"), bg.name(QColor::HexRgb));
    css.replace(QLatin1String("@border@"), border.name(QColor::HexRgb));
    css.replace(QLatin1String("@text@"), text.name(QColor::HexRgb));
    css.replace(QLatin1String("@hover@"), hover.name(QColor::HexRgb));
    css.replace(QLatin1String("@radius@"), QString::number(int(controlRadius())));

    if (button->styleSheet() != css)
        button->setStyleSheet(css);
}
