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
#include <QPixmap>

/** Qt5 stylesheets cache url() images by path; a color-specific name avoids a
 *  stale chevron surviving a light/dark switch. */
static QString chevronPath(const QColor &c){
    const QString path = QDir::temp().filePath(
        QStringLiteral("eiskalt-combo-chevron-%1.png").arg(c.name(QColor::HexRgb).mid(1)));
    if (!QFile::exists(path)) {
        QPixmap pm(12, 8);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawPolygon(QPolygonF() << QPointF(2, 2) << QPointF(10, 2) << QPointF(6, 6));
        pm.save(path, "PNG");
    }
    return path;
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
    return css.replace(QLatin1String("@chevron@"), chevronPath(c.text));
}
