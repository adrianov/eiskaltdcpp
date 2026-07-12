/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "AppTheme.h"

#include <QDir>
#include <QFile>
#include <QPainter>
#include <QPen>
#include <QPixmap>

/** Qt5 stylesheets cache url() images by path; a color-specific name avoids a
 *  stale glyph surviving a light/dark switch. */
static QString glyphPath(const QString &kind, int w, int h, const QColor &c,
                         void (*paint)(QPainter &, const QColor &)){
    const QString path = QDir::temp().filePath(
        QStringLiteral("eiskalt-%1-%2.png").arg(kind, c.name(QColor::HexRgb).mid(1)));
    if (!QFile::exists(path)) {
        QPixmap pm(w, h);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);
        paint(p, c);
        pm.save(path, "PNG");
    }
    return path;
}

static void paintChevron(QPainter &p, const QColor &c){
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    p.drawPolygon(QPolygonF() << QPointF(2, 2) << QPointF(10, 2) << QPointF(6, 6));
}

static void paintCheck(QPainter &p, const QColor &c){
    QPen pen(c, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.drawPolyline(QPolygonF() << QPointF(3, 7.5) << QPointF(6, 10.5) << QPointF(11, 3.5));
}

static void paintRadioDot(QPainter &p, const QColor &c){
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    p.drawEllipse(QRectF(4, 4, 6, 6));
}

static QString toggleStyleSheet(const AppTheme::InputColors &c){
    // 14px indicators need an opaque stroke; inputBorder()'s dark alpha vanishes in HexRgb
    // only by accident — force alpha so unchecked boxes stay visible on dark Window.
    QColor border = AppTheme::inputBorder(false);
    border.setAlpha(255);
    QColor disabledOn = c.muted;
    if (disabledOn.lightness() > 128)
        disabledOn = disabledOn.darker(180);
    const QColor mark = AppTheme::readableColor(c.selBg, QColor(Qt::white));
    QString css = QStringLiteral(
        "QCheckBox::indicator, QRadioButton::indicator {"
        "  width: 14px; height: 14px;"
        "  border: 1px solid %1;"
        "  background: %2;"
        "}"
        "QCheckBox::indicator { border-radius: 3px; }"
        "QRadioButton::indicator { border-radius: 7px; }"
        "QCheckBox::indicator:hover, QRadioButton::indicator:hover { border-color: %3; }"
        "QCheckBox::indicator:checked, QRadioButton::indicator:checked {"
        "  border-color: %3; background: %3;"
        "}"
        "QCheckBox::indicator:checked { image: url(@check@); }"
        "QRadioButton::indicator:checked { image: url(@radio@); }"
        "QCheckBox::indicator:disabled, QRadioButton::indicator:disabled {"
        "  border-color: %4; background: %2;"
        "}"
        "QCheckBox::indicator:checked:disabled, QRadioButton::indicator:checked:disabled {"
        "  border-color: %4; background: %5;"
        "}")
        .arg(border.name(QColor::HexRgb),
             c.bg.name(QColor::HexRgb),
             c.selBg.name(QColor::HexRgb),
             c.muted.name(QColor::HexRgb),
             disabledOn.name(QColor::HexRgb));
    css.replace(QLatin1String("@check@"), glyphPath(QStringLiteral("check"), 14, 14, mark, paintCheck));
    css.replace(QLatin1String("@radio@"), glyphPath(QStringLiteral("radio"), 14, 14, mark, paintRadioDot));
    return css;
}

QString AppTheme::comboStyleSheet(){
    const InputColors c = inputColors();

    QString css = QStringLiteral(
        "QComboBox {"
        "  border: 1px solid %3;"
        "  border-radius: %7px;"
        "  padding: 2px 18px 2px 8px;"
        "  background: %1;"
        "  color: %2;"
        "  min-height: 1.2em;"
        "}"
        "QComboBox:disabled { color: %8; }"
        "QComboBox:focus { border: 1.5px solid %4; }"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: center right;"
        "  border: none;"
        "  width: 18px;"
        "}"
        "QComboBox::down-arrow {"
        "  image: url(@chevron@);"
        "  width: 12px;"
        "  height: 8px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: %1;"
        "  color: %2;"
        "  selection-background-color: %5;"
        "  selection-color: %6;"
        "  outline: 0;"
        "}")
        .arg(c.bg.name(QColor::HexRgb),
             c.text.name(QColor::HexRgb),
             inputBorder(false).name(QColor::HexRgb),
             inputBorder(true).name(QColor::HexRgb),
             c.selBg.name(QColor::HexRgb),
             c.selFg.name(QColor::HexRgb),
             QString::number(int(controlRadius())),
             c.muted.name(QColor::HexRgb));
    css.replace(QLatin1String("@chevron@"),
                glyphPath(QStringLiteral("chevron"), 12, 8, c.text, paintChevron));
    return css + toggleStyleSheet(c);
}
