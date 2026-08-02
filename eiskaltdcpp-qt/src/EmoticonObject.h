/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QString>
#include <QMap>
#include <QList>
#include <QStringList>
#include <QPixmap>
#include <QLabel>
#include <QMouseEvent>

// Legacy on-screen size for hi-res packs (e.g. 72px default theme assets).
static const int EMOTICON_LOGICAL_SIDE = 24;

// Keep small packs at native size; downscale only when larger than 24.
// Divide by DPR so a already-scaled pixmap is not treated as native pixels.
static inline int emoticonLogicalSide(const QPixmap &source)
{
    if (source.isNull())
        return EMOTICON_LOGICAL_SIDE;
    const qreal dpr = qMax(qreal(1), source.devicePixelRatio());
    const int side = qMax(qRound(source.width() / dpr), qRound(source.height() / dpr));
    return qMin(side, EMOTICON_LOGICAL_SIDE);
}

class EmoticonLabel: public QLabel{
Q_OBJECT
public:
    EmoticonLabel(QWidget *parent = nullptr) : QLabel(parent){}
    virtual ~EmoticonLabel(){}

Q_SIGNALS:
    void clicked();

protected:
    virtual void mousePressEvent(QMouseEvent *ev){
        QLabel::mousePressEvent(ev);

        emit clicked();
    }
};

struct EmoticonObject{
    QString fileName;
    QPixmap pixmap;

    int id;
};

typedef QMap<QString, EmoticonObject*> EmoticonMap;
typedef QList<EmoticonObject*> EmoticonList;
