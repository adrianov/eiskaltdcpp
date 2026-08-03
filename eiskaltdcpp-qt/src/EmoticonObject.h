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

// Chat + smile-selector size: always 20 virtual (logical) pixels.
// Physical pixels come from WulforUtil::scalePixmap(source, 20):
//   @2x → 40, @3x → 60, @4x → 80 (rare). Hi-res 240px packs stay sharp.
static const int EMOTICON_LOGICAL_SIDE = 20;

static inline int emoticonLogicalSide()
{
    return EMOTICON_LOGICAL_SIDE;
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
