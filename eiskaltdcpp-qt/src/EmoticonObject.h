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
#include <QMargins>

// On-screen size for hi-res packs (e.g. 72px default theme assets).
// 20 matches visual weight of older padded 24px art more closely than a full 24 box.
static const int EMOTICON_LOGICAL_SIDE = 20;

// Keep small packs at native size; downscale only when larger than the cap.
// Divide by DPR so an already-scaled pixmap is not treated as native pixels.
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

    // FlowLayout sizes from sizeHint; never use QPixmap::size() (physical on Retina).
    QSize sizeHint() const override {
        const QPixmap px = pixmap(Qt::ReturnByValue);
        if (px.isNull())
            return QLabel::sizeHint();
        const qreal dpr = qMax(qreal(1), px.devicePixelRatio());
        const QSize logical(qRound(px.width() / dpr), qRound(px.height() / dpr));
        const QMargins m = contentsMargins();
        return logical + QSize(m.left() + m.right(), m.top() + m.bottom());
    }
    QSize minimumSizeHint() const override { return sizeHint(); }

Q_SIGNALS:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *ev) override {
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
