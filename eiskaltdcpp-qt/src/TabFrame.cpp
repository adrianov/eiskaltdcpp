/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TabFrame.h"

#include "FlowLayout.h"
#include "TabButton.h"
#include "WulforUtil.h"
#include "ArenaWidgetManager.h"
#include "DebugHelper.h"
#include "GlobalTimer.h"

#if QT_VERSION >= 0x050000
#include <QtWidgets>
#else
#include <QtGui>
#endif

#include <QPushButton>
#include <QWheelEvent>
#include <functional>

TabFrame::TabFrame(QWidget *parent) :
    QFrame(parent)
{
    DEBUG_BLOCK
    
    setAcceptDrops(true);

    fr_layout = new FlowLayout(this);
    fr_layout->setContentsMargins(0, 0, 0, 0);

    setMinimumHeight(20);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    shortcuts << (new QShortcut(QKeySequence(Qt::ALT + Qt::Key_1), this))
              << (new QShortcut(QKeySequence(Qt::ALT + Qt::Key_2), this))
              << (new QShortcut(QKeySequence(Qt::ALT + Qt::Key_3), this))
              << (new QShortcut(QKeySequence(Qt::ALT + Qt::Key_4), this))
              << (new QShortcut(QKeySequence(Qt::ALT + Qt::Key_5), this))
              << (new QShortcut(QKeySequence(Qt::ALT + Qt::Key_6), this))
              << (new QShortcut(QKeySequence(Qt::ALT + Qt::Key_7), this))
              << (new QShortcut(QKeySequence(Qt::ALT + Qt::Key_8), this))
              << (new QShortcut(QKeySequence(Qt::ALT + Qt::Key_9), this))
              << (new QShortcut(QKeySequence(Qt::ALT + Qt::Key_0), this));

    for (const auto &s : shortcuts){
        s->setContext(Qt::ApplicationShortcut);

        connect(s, SIGNAL(activated()), this, SLOT(slotShorcuts()));
    }

    connect(GlobalTimer::getInstance(), SIGNAL(second()), this, SLOT(redraw()));
}


TabFrame::~TabFrame(){
    DEBUG_BLOCK

    for (const auto &key : tbtn_map.keys()){
        TabButton *btn = const_cast<TabButton*>(key);

        btn->deleteLater();
    }
}

void TabFrame::resizeEvent(QResizeEvent *e){
    e->accept();

    QFrame::updateGeometry();
}

bool TabFrame::eventFilter(QObject *obj, QEvent *e){
    TabButton *btn = qobject_cast<TabButton*>(obj);
    QWheelEvent *w_e = reinterpret_cast<QWheelEvent*>(e);

    if (btn && (e->type() == QEvent::Wheel) && w_e){
        const int dy = wulforWheelDeltaY(w_e);
        int numDegrees = (dy < 0)? (-1*dy/8) : (dy/8);
        int numSteps = numDegrees/15;
        std::function<void()> f = [this]() { this->nextTab(); };

        if (dy < 0)
            f = [this]() { this->prevTab(); };

        for (int i = 0; i < numSteps; i++)
            f();

        return true;
    }

    return QFrame::eventFilter(obj, e);
}

QSize TabFrame::sizeHint() const {
    QSize s(fr_layout->sizeHint().width() , fr_layout->heightForWidth(width()));
    return s;
}

QSize TabFrame::minimumSizeHint() const{
    return sizeHint();
}

